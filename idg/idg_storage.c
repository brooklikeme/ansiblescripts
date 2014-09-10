/*
 *  idg_storage.c
 *  IDG test storage
 *  Created on: Nov 4, 2013
 *      Author: qiuxi.zhang
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "idg_common.h"
#include "idg_storage.h"

// buffer
client_t clients[CLIENT_COUNT];
int socket_listener;

int storage_running = 1;
pthread_t send_threads[STORAGE_SEND_THREAD_COUNT];
pthread_t recv_threads[STORAGE_RECV_THREAD_COUNT];

//
void init_storage()
{
    int i = 0;
    // init client list
    for (i = 0; i < CLIENT_COUNT; i ++) {
        clients[i].sockfd = -1;
        pthread_mutex_init(&clients[i].send_buffer.socket_mutex, NULL);
        pthread_mutex_init(&clients[i].send_buffer.status_mutex, NULL);        
    }    
}

//
void destory_storage()
{
    int i;
    close(socket_listener);
    for (i = 0; i < CLIENT_COUNT; i ++) {
        pthread_mutex_destroy(&clients[i].send_buffer.status_mutex);
        pthread_mutex_destroy(&clients[i].send_buffer.socket_mutex);        
    }    
}

//
int init_listener(char *ip_address)
{
    socket_listener = -1;
    struct addrinfo flags;
    struct addrinfo *host_info;

    char port_str[10];
    bzero(port_str, sizeof(port_str));
    sprintf(port_str, "%d", STORAGE_PORT);    
    
    memset(&flags, 0, sizeof(flags));
    flags.ai_family = AF_UNSPEC; 
    flags.ai_socktype = SOCK_STREAM;
    flags.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip_address, port_str, &flags, &host_info) < 0) {
        perror("Couldn't read host info for socket start");
        return -1;
    }
    
    socket_listener = socket(host_info->ai_family, host_info->ai_socktype, host_info->ai_protocol);    
    if (socket_listener < 0) {
        perror("Error opening socket");
        return -1;
    }
        
    int yes = 1, on = 1;         
    
    if (setsockopt(socket_listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) < 0) {                     
        perror("ERROR setsockopt");
        return -1;
    }        
    
    if (ioctl(socket_listener, FIONBIO, (char *)&on) < 0) {
       perror("ERROR ioctl");
       return -1;
    }
    
    if (bind(socket_listener, host_info->ai_addr, host_info->ai_addrlen) < 0) {
        perror("Error on binding");
        return -1;        
    }

    freeaddrinfo(host_info); 
    
    if (listen(socket_listener, QUEUE_SIZE) < 0) {
        close(socket_listener);
        perror("listen");
        return -1;
    }
    
    return socket_listener;
}

//
void* recv_thread_routine(void *arg)
{
    int i, ret;
    char read_buffer[SOCK_PACKET_SIZE];    
    TReportHead report_head;    
    while (storage_running) {
        for (i = 0; i < CLIENT_COUNT; i ++) {
            if (clients[i].sockfd < 0 || !recv_check_and_lock(&clients[i].send_buffer)) {
                usleep(100);
                continue;                
            }
            if (check_recv_status(clients[i].sockfd) > 0) {
                // read head
                bzero(&report_head, sizeof(report_head));
                if ((ret = recv_data_non_block(clients[i].sockfd, &report_head, sizeof(TReportHead), 0, RECV_TIMEOUT, 0)) <= 0) {
                    printf("recv_thread_routine() recv head error, ip = %s, sockfd = %d, ret = %d\n", clients[i].ip_address,  clients[i].sockfd, ret);   
                    fflush(stderr);                                                               
                    fflush(stdout);                                                                                
                    close(clients[i].sockfd);
                    clients[i].sockfd = -1;
                    recv_unlock(&clients[i].send_buffer);    
                    continue;
                }
                if (report_head.type == 0) {
                    // add send head
                    add_send_head(&clients[i].send_buffer, report_head.index, report_head.count);
                } else {
                    if ((ret = recv_data_non_block(clients[i].sockfd, read_buffer, report_head.count * sizeof(TReportItem), 1, RECV_TIMEOUT, 0)) <= 0) {
                        printf("recv_thread_routine() recv items error, ip = %s, sockfd = %d, ret = %d\n", clients[i].ip_address,  clients[i].sockfd, ret);                                            
                        fflush(stderr);                                                               
                        fflush(stdout);                                                                                
                        close(clients[i].sockfd);
                        clients[i].sockfd = -1;
                        recv_unlock(&clients[i].send_buffer);
                        continue;
                    }                    
                }
                recv_unlock(&clients[i].send_buffer);
                                
                // quit loop
                break;
            } else {
                recv_unlock(&clients[i].send_buffer);
            }
        }
        usleep(100);
    }
    // return normally    
    pthread_exit(0);
}

//
void* send_thread_routine(void *arg)
{
    int i, ret;
    char write_buffer[SOCK_PACKET_SIZE];
    PReportHead report_head;
    TReportHead send_head;
    while (storage_running) {
        for (i = 0; i < CLIENT_COUNT; i ++) {
            if (clients[i].sockfd < 0 || !send_check_and_lock(&clients[i].send_buffer)) {
                usleep(100);
                continue;                
            }
            if ((report_head = fetch_send_head(&clients[i].send_buffer)) != NULL) {
                send_head.type = 1;
                send_head.index = report_head->index;
                while (report_head->count > 0) {
                    if (report_head->count > REPORT_MTU_COUNT) {
                        send_head.count = REPORT_MTU_COUNT;
                        report_head->count -= REPORT_MTU_COUNT;
                    } else {
                        send_head.count = report_head->count;
                        report_head->count = 0;
                    }
                    // send head data
                    ret = send_data_non_block(clients[i].sockfd, &send_head, sizeof(TReportHead), 0, SEND_TIMEOUT, 0);
                    if (ret <= 0) {
                        printf("send_thread_routine() send head error, ip = %s, sockfd = %d, ret = %d, count = %d\n", clients[i].ip_address,  clients[i].sockfd, ret, send_head.count);
                        fflush(stderr);                                                               
                        fflush(stdout);                                                                                
                        close(clients[i].sockfd);
                        clients[i].sockfd = -1;
                        break;
                    }                
                    // send items
                    if (send_data_non_block(clients[i].sockfd, write_buffer, sizeof(TReportItem) * send_head.count, 1, SEND_TIMEOUT, 0) <= 0) {
                        printf("send_thread_routine() send items error, ip = %s, sockfd = %d, ret = %d, count = %d\n", clients[i].ip_address,  clients[i].sockfd, ret, send_head.count);                                            
                        fflush(stderr);                                                               
                        fflush(stdout);                                                                                
                        close(clients[i].sockfd);
                        clients[i].sockfd = -1;
                        break;
                    }                                    
                }                
                send_unlock(&clients[i].send_buffer);                
                break;
            } else {
                send_unlock(&clients[i].send_buffer);
            }
        }
        usleep(100);
    }
    // return normally    
    pthread_exit(0);  
}

// main function
int main(int argc, char *argv[])
{   
    int i;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [ip address]\n", argv[0]);
        exit(-1);
    }
    
    // init data
    init_storage();
    
    // init socket listener
    if (init_listener(argv[1]) <= 0) {
        exit(-1);
    }
    
    printf("*********************************************************************\n");    
    printf("idg storage is running...\n");
    
    // set up threads
    for (i = 0; i < STORAGE_RECV_THREAD_COUNT; i++) {
        int *recv_index = malloc(sizeof(int));
        *recv_index = i;        
        pthread_create(&recv_threads[i], NULL, &recv_thread_routine, (void *) recv_index);
    }                            
    for (i = 0; i < STORAGE_SEND_THREAD_COUNT; i++) {
        int *send_index = malloc(sizeof(int));
        *send_index = i;        
        pthread_create(&send_threads[i], NULL, &send_thread_routine, (void *) send_index);
    }                            
    
    int ret, max_sd = 0, new_sd;
    fd_set recv_set;
    
    struct timeval curr_time;
    char last_ip_address[INET6_ADDRSTRLEN];
    long long last_accept_time = 0, time_interval = 0;
        
    struct timeval timeout_read;  
    
    while (1) {
        timeout_read.tv_sec  = 3600;
        timeout_read.tv_usec = 0;
        
        gettimeofday(&curr_time, NULL);
        if (last_accept_time > 0) {
            time_interval = curr_time.tv_sec * 1000000 + curr_time.tv_usec - last_accept_time;            
            printf("**** main() last accept ip = %s, interval = %lld us\n", last_ip_address, time_interval);
        }
        last_accept_time = curr_time.tv_sec * 1000000 + curr_time.tv_usec;        
        // Initialize the master fd_set
        FD_ZERO(&recv_set);
        FD_SET(socket_listener, &recv_set);
        max_sd = socket_listener;
        ret = select(max_sd + 1, &recv_set, NULL, NULL, &timeout_read);
        if (ret < 0) {
            perror("select() failed");
            goto loop_error;
        } else if (ret == 0) {
            printf("select() timeout\n");  
            goto loop_error;          
        } else if (ret > 0) {
            if (FD_ISSET(socket_listener, &recv_set)) {
                struct sockaddr_in their_addr;
                socklen_t size = sizeof(struct sockaddr_in);
                new_sd = accept(socket_listener, (struct sockaddr*)&their_addr, &size);
                if (new_sd < 0) {
                    if (errno != EWOULDBLOCK) {
                        perror("Error on accept");
                        goto loop_error;
                    } else {
                        break;
                    }                   
                } else {
                    printf("---------------------------------------------------------------------\n");    
                    printf("New connection from %s on port %d\n",
                        inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port));
                    fflush(stderr);                                                               
                    fflush(stdout);                                                                                                        
                    // set buffer size
                    int yes = 1, on = 1;             
                    if (setsockopt(new_sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {                     
                        perror("ERROR setsockopt");
                        return -1;
                    }        

                    if (ioctl(new_sd, FIONBIO, (char *)&on) < 0) {
                       perror("ERROR ioctl");
                       return -1;
                    }
                                        
                    // mutex area
                    int client_full = 1;
                                    
                    for (i = 0; i < CLIENT_COUNT; i ++) {
                        pthread_mutex_lock(&clients[i].send_buffer.status_mutex);
                        if (clients[i].sockfd == -1) {
                            clients[i].sockfd = new_sd;
                            bzero(&clients[i].send_buffer, sizeof(clients[i].send_buffer));
                            strcpy(clients[i].ip_address, inet_ntoa(their_addr.sin_addr)); 
                            strcpy(last_ip_address, clients[i].ip_address);
                            // create thread 
                            client_full = 0;
                            pthread_mutex_unlock(&clients[i].send_buffer.status_mutex);            
                            break;            
                        }
                        pthread_mutex_unlock(&clients[i].send_buffer.status_mutex);
                    }    

                    if (client_full == 1) {
                        printf("Connection pool is full, pool size: [%d]\n", CLIENT_COUNT);
                        close(new_sd);
                    }
                }
            }
        } 
    }
loop_error:
    // Cleanup all of the sockets that are open
    for (i = 0; i < CLIENT_COUNT; i ++) {
        if (clients[i].sockfd > -1) {
            close(clients[i].sockfd);
        }
    }

    // wait for threads
    storage_running = 0;
    // wait for recv thread
    for (i = 0; i < STORAGE_RECV_THREAD_COUNT; i ++) {
        pthread_join(recv_threads[i], NULL);
    }                        
    // wait for send threads          
    for (i = 0; i < STORAGE_SEND_THREAD_COUNT; i ++) {
        pthread_join(send_threads[i], NULL);
    }                        
    
    // destory variable
    destory_storage();
    
    return 0;
}
