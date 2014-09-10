/*
 *  idg_client.c
 *  IDG test client
 *  Created on: Nov 4, 2013
 *      Author: qiuxi.zhang
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h> 

#include "idg_client.h"

// buffer
int server_socket;
storage_t storages[STORAGE_COUNT];

TIndexHead index_head;
PIndexItem index_items = NULL;

int send_running = 1;
int recv_running = 1;

pthread_t send_threads[CLIENT_SEND_THREAD_COUNT];
pthread_t recv_threads[CLIENT_RECV_THREAD_COUNT];

//
int init_client()
{
    int i;
    bzero(storages, sizeof(storages));
    
    for (i = 0; i < STORAGE_COUNT; i ++) {
        storages[i].sockfd = -1;
        pthread_mutex_init(&storages[i].send_buffer.status_mutex, NULL);
        pthread_mutex_init(&storages[i].send_buffer.socket_mutex, NULL);
    }
    
    // get storage address array
    char buffer[1024] = STORAGE_ADDRESS;
    const char sep[2] = ",";
    char *token;   
    /* get the first token */
    token = strtok(buffer, sep);
   
    int index = 0;
    /* walk through other tokens */
    while (index < STORAGE_COUNT && NULL != token) {
       strcpy(storages[index].ip_address, token);    
       token = strtok(NULL, sep);
       index ++;
    }    
    
    if (index != STORAGE_COUNT) {
        printf("Error reading storage address: %s\n", STORAGE_ADDRESS);
        return -1;        
    }
        
    return 0;  
}

//
void destory_client()
{
    int i;
    if (NULL != index_items)
        free (index_items);
    for (i = 0; i < STORAGE_COUNT; i ++) {
        pthread_mutex_destroy(&storages[i].send_buffer.status_mutex);
        pthread_mutex_destroy(&storages[i].send_buffer.socket_mutex);        
    }        
}

//
int init_server_socket()
{
    server_socket = -1;
    struct sockaddr_in serv_addr;   
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error opening socket");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_ADDRESS, &serv_addr.sin_addr) <= 0) {
        perror("ERROR net_pton");
        server_socket = -1;
        return -1;
    }
    
    time_t now = time((time_t *)NULL);    
    printf("[%s] Connecting to server begins...non blocking status = %d\n", get_string_from_time(now), is_non_blocking(server_socket));
    fflush(stdout);
    int ret = connect(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        if (errno == EINPROGRESS) {
            now = time((time_t *)NULL);
            printf("[%s] connect IN PROGRESS!\n", get_string_from_time(now));
        } else {
            perror("Error connect");
            close(server_socket);
            server_socket = -1;
            return -1;            
        }
    }
    
    now = time((time_t *)NULL);
    printf("---------------------------------------------------------------------\n");        
    printf("[%s] Server socket established!\n", get_string_from_time(now));
    fflush(stdout);    
        
    return 0;
}

int index_item_comp(const void *item1, const void *item2)
{
    return ((PIndexItem)item1)->index1 == ((PIndexItem)item2)->index1 ?
        ((PIndexItem)item1)->index2 - ((PIndexItem)item2)->index2 
        : ((PIndexItem)item1)->index1 - ((PIndexItem)item2)->index1;
}

//
int read_index_data()
{
    int i;    
    // read head
    bzero(&index_head, sizeof(index_head));
    time_t now = time((time_t *)NULL);
    printf("[%s] begin receive index head...\n", get_string_from_time(now));        
    fflush(stdout);    
    long long n_read = recv_data_block(server_socket, &index_head, sizeof(TIndexHead), 0);
    if (n_read <= 0) {
        close(server_socket);
        return -1;
    }
    now = time((time_t *)NULL);    
    printf("[%s] process starting... (index count = %d, test = %s)\n", get_string_from_time(now), index_head.count, 
        index_head.is_test ? "true" : "false");        
    fflush(stdout);
    
    // read items
    index_items = calloc(index_head.count, sizeof(TIndexItem));
    n_read = recv_data_block(server_socket, index_items, index_head.count * sizeof(TIndexItem), 0);
    if (n_read <= 0) {
        close(server_socket);
        return -1;
    }
    
    // sort index data by index1 and 2 
    qsort(index_items, index_head.count, sizeof(TIndexItem), index_item_comp);    
    
    // get range parameters
    int old_index;
    for (i = 0; i < index_head.count; i ++) {
        if (i == 0) {
            old_index = index_items[i].index1;
        }
        if (i > 0 && index_items[i].index1 != old_index) {
            storages[old_index].range.thru = i - 1;
            storages[old_index].range.count = storages[old_index].range.thru - storages[old_index].range.from + 1;
            storages[old_index].test_recv_count = storages[old_index].range.count;
            old_index = index_items[i].index1;
            storages[old_index].range.from = i;
        }
        if (i == index_head.count - 1) {
            storages[old_index].range.thru = i;
            storages[old_index].range.count = storages[old_index].range.thru - storages[old_index].range.from + 1;
            storages[old_index].test_recv_count = storages[old_index].range.count;            
        }
        // set test 
        storages[index_items[i].index2].test_send_count ++;
    }
    // DEBUG
    for (i = 0; i < STORAGE_COUNT; i ++) {
        printf("DEBUG order = %d from=%d thru=%d count=%d test_recv_count=%d test_send_count=%d\n", i, 
            storages[i].range.from, storages[i].range.thru, storages[i].range.count, storages[i].test_recv_count, storages[i].test_send_count);
    }
    
    return 0;
}

//
int wait_for_starting()
{
    // send storage conn status
    TProgress progress;
    progress.state = 8888;
    long long n_send = send_data_block(server_socket, &progress, sizeof(TProgress), 0);
    if (n_send <= 0) {
        close(server_socket);
        return -1;
    }    
    
    // wait for starting
    long long n_read = recv_data_block(server_socket, &progress, sizeof(TProgress), 0);
    if (n_read <= 0) {
        close(server_socket);
        return -1;
    }
    
    return 0;
}

//
void* send_thread_routine(void *arg)
{
    int *p_thread_id = (int *) arg;
    int thread_id = *p_thread_id;
    free (p_thread_id);    
    int i, send_count, ret;
    TProgress progress;
    char write_buffer[SOCK_PACKET_SIZE];
    bzero(&progress, sizeof(progress));
    PReportHead report_head;
    while (send_running) {        
        for (i = 0; i < STORAGE_COUNT; i ++) {
            if (storages[i].sockfd < 0 || !send_check_and_lock(&storages[i].send_buffer)) {
                usleep(100);                
                continue;
            }            
            if ((report_head = fetch_send_head(&storages[i].send_buffer)) != NULL) {
                // send data
                if ((ret = send_data_non_block(storages[i].sockfd, report_head, sizeof(TReportHead), 0, SEND_TIMEOUT, 0)) <= 0) {
                    printf("send_thread_routine() send to recv head error, ip = %s, sock = %d, count = %d, thread_id = %d\n", storages[i].ip_address, storages[i].sockfd, report_head->count, thread_id);                    
                    close(storages[i].sockfd);
                    storages[i].sockfd = -1;
                    send_unlock(&storages[i].send_buffer);
                    pthread_exit(0);                    
                }
                // reset head count
                report_head->count = 0;
                send_unlock(&storages[i].send_buffer);
                break;
            } else if ((send_count = fetch_send_count(&storages[i].send_buffer)) > 0) {
                // send head
            	TReportHead head;
                head.type = 1;
                head.count = send_count;
                head.index = 8888;
                if ((ret = send_data_non_block(storages[i].sockfd, &head, sizeof(TReportHead), 0, SEND_TIMEOUT, 0)) <= 0) {
                    printf("send_thread_routine() send head error, ip = %s, sock = %d, ret = %d, count = %d, thread_id = %d\n", storages[i].ip_address, storages[i].sockfd, ret, send_count, thread_id);
                    close(storages[i].sockfd);
                    storages[i].sockfd = -1;
                    send_unlock(&storages[i].send_buffer);                    
                    pthread_exit(0);                    
                }                
                // send item
                if ((ret = send_data_non_block(storages[i].sockfd, write_buffer, sizeof(TReportItem) * send_count, 1, SEND_TIMEOUT, 0)) <= 0) {
                    printf("send_thread_routine() send items error, ip = %s, sock = %d, ret = %d, count = %d, thread_id = %d\n", storages[i].ip_address, storages[i].sockfd, ret, send_count, thread_id);
                    fflush(stderr);                                                               
                    fflush(stdout);                                                                                                    
                    close(storages[i].sockfd);
                    storages[i].sockfd = -1;
                    send_unlock(&storages[i].send_buffer);
                    pthread_exit(0);                    
                }   
                storages[i].send_buffer.send_count += send_count;
                
                send_unlock(&storages[i].send_buffer);                
                break;                               
            } else {
                send_unlock(&storages[i].send_buffer);
            }
        }
        usleep(100);
    }
    // return normally    
    pthread_exit(0);
}

//
void* recv_thread_routine(void *arg)
{
    int i, ret;
    char read_buffer[SOCK_PACKET_SIZE];    
    TReportHead report_head;    
    while (recv_running) {
        for (i = 0; i < STORAGE_COUNT; i ++) {
            if (storages[i].sockfd < 0 || !recv_check_and_lock(&storages[i].send_buffer)) {
                usleep(100);                
                continue;
            }            
            if (check_recv_status(storages[i].sockfd) > 0) {
                // read head
                bzero(&report_head, sizeof(report_head));
                if ((ret = recv_data_non_block(storages[i].sockfd, &report_head, sizeof(TReportHead), 0, RECV_TIMEOUT, 0)) <= 0) {
                    printf("recv_thread_routine() recv head error, ip = %s, sock = %d, ret = %d\n", storages[i].ip_address, storages[i].sockfd, ret);
                    fflush(stderr);                                                               
                    fflush(stdout);                                                                                                    
                    close(storages[i].sockfd);
                    storages[i].sockfd = -1;
                    recv_unlock(&storages[i].send_buffer);                    
                    pthread_exit(0);
                }
                // read items
                if ((ret = recv_data_non_block(storages[i].sockfd, read_buffer, report_head.count * sizeof(TReportItem), 1, RECV_TIMEOUT, 0)) <= 0) {
                    printf("recv_thread_routine() recv items error, ip = %s, sock = %d, ret = %d\n", storages[i].ip_address, storages[i].sockfd, ret);                    
                    fflush(stderr);                                                               
                    fflush(stdout);                                                                                                    
                    close(storages[i].sockfd);
                    storages[i].sockfd = -1;
                    recv_unlock(&storages[i].send_buffer);                    
                    pthread_exit(0);
                }                
                // change buffer count
                add_send_count(&storages[report_head.index].send_buffer, report_head.count);
                // change buffer count
                add_recv_count(&storages[i].send_buffer, report_head.count);                
                
                recv_unlock(&storages[i].send_buffer);
                                
                // quit loop
                break;
            } else {
                recv_unlock(&storages[i].send_buffer);
            }
        }
        usleep(100);
    }
    // return normally    
    pthread_exit(0);
}

//
int init_storages()
{
    int i, j, error, len;   
    int yes = 1, on = 1;     
    fd_set write_set;  
    struct timeval time_val;    
    int connect_ret;    
    struct timeval curr_time;
    char last_ip_address[INET6_ADDRSTRLEN];
    long long last_connect_time = 0, time_interval = 0;    
    
    // set up storage sockets    
    for (i = 0; i < STORAGE_COUNT; i++) {
        connect_ret = -1;
        struct sockaddr_in serv_addr;   
        storages[i].sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (storages[i].sockfd < 0) {
            perror("Error opening socket");
            for (j = 0; j < i; j ++) {
                close(storages[j].sockfd);
            }
            return -1;
        }

        memset(&serv_addr, '0', sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(STORAGE_PORT);

        if (inet_pton(AF_INET, storages[i].ip_address, &serv_addr.sin_addr) <= 0) {
            perror("ERROR net_pton");
            for (j = 0; j < i; j ++) {
                close(storages[j].sockfd);
            }            
            return -1;
        }

        if (setsockopt(storages[i].sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
            perror("ERROR : setsockopt");
            for (j = 0; j < i; j ++) {
                close(storages[j].sockfd);
            }                        
            return -1;
        }

        if (ioctl(storages[i].sockfd, FIONBIO, (char *)&on) < 0) {
           close(storages[i].sockfd);
           for (j = 0; j < i; j ++) {
               close(storages[j].sockfd);
           }                       
           perror("ERROR : ioctl");
           return -1;
        }
        
        // do 
        gettimeofday(&curr_time, NULL);
        if (last_connect_time > 0) {
            time_interval = curr_time.tv_sec * 1000000 + curr_time.tv_usec - last_connect_time;            
            printf("**** init_storages() last connect ip = %s, interval = %lld us\n", last_ip_address, time_interval);
        }
        last_connect_time = curr_time.tv_sec * 1000000 + curr_time.tv_usec;        
        
        int ret = connect(storages[i].sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret < 0) {
            if (errno == EINPROGRESS) {
                printf("init_storages() connect() connecting to %s, sock = %d\n", storages[i].ip_address, storages[i].sockfd);
                FD_ZERO(&write_set);  
                FD_SET(storages[i].sockfd, &write_set);  
                time_val.tv_sec = CONNECT_TIMEOUT;
                time_val.tv_usec = 0;                
                ret = select(storages[i].sockfd + 1, NULL, &write_set, NULL, &time_val);
                if (ret < 0) {
                    printf("init_storages() select() ERROR connecting to %s, code-msg:%d - %s\n", storages[i].ip_address, errno, strerror(errno));
                    connect_ret = -1;
                } else if (ret == 0) {
                    printf("init_storages() select() TIMEOUT connecting to %s, code-msg:%d - %s\n", storages[i].ip_address, errno, strerror(errno));
                } else if (FD_ISSET(storages[i].sockfd, &write_set)) {  
                    len = sizeof(int);  
                    ret = getsockopt(storages[i].sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);  
                    if (ret < 0 || error) {  
                        printf("init_storages() getsockopt() ERROR connecting to %s, code-msg:%d - %s\n", storages[i].ip_address, errno, strerror(errno));                            
                    } else {
                        connect_ret = 0;
                    }  
                } else {  
                    printf("init_storages() select() connecting to %s, sockfd not set\n", storages[i].ip_address);
                }    
            }
        } else {
            connect_ret = 0;
        }                   
        if (connect_ret < 0) {
            for (j = 0; j < i; j ++) {
                close(storages[j].sockfd);
            }            
            close(storages[i].sockfd);
            return -1;                            
        }
        strcpy(last_ip_address, storages[i].ip_address);        
        
        // init send buffer
        int old_index;                
        int send_count = 0;    
        for (j = storages[i].range.from; j <= storages[i].range.thru; j ++) {
            send_count ++;        
            if (j == storages[i].range.from) {
                old_index = index_items[j].index2;
            }
            if (index_items[j].index2 != old_index) {        
                // add head                
                add_send_head(&storages[i].send_buffer, old_index, send_count - 1);                
                storages[old_index].send_buffer.to_send += send_count - 1;
                old_index = index_items[j].index2;
                send_count = 1;
            }
            if (j == storages[i].range.thru && send_count > 0) {
                add_send_head(&storages[i].send_buffer, old_index, send_count);
                storages[old_index].send_buffer.to_send += send_count;
                send_count = 0;   
            }
        }        
    }
    
    return 0;
}

// main function
int main(int argc, char *argv[])
{        
    time_t now;
    if (init_client() < 0) {
        exit(-1);
    }
    
    TProgress progress;

    printf("*********************************************************************\n");
    printf("idg client is running...\n");
    // loop to get task from server
    while (1) {
        // init socket connection
        if (init_server_socket() >= 0 
            && read_index_data() >= 0
            && init_storages() >=0
            && wait_for_starting() >=0)
        {
            int has_error = 0;
            printf("connect to server suceess, processing begins...\n");            
            fflush(stdout);
            fflush(stderr);
            int i;
            time_t begin_time = time((time_t *)NULL);

            // create threads         
            recv_running = 1;
            send_running = 1;
            // set up storage threads
            for (i = 0; i < CLIENT_RECV_THREAD_COUNT; i++) {
                int *recv_index = malloc(sizeof(int));
                *recv_index = i;                
                pthread_create(&recv_threads[i], NULL, &recv_thread_routine, (void *) recv_index);
            }                                    
            for (i = 0; i < CLIENT_SEND_THREAD_COUNT; i++) {
                int *send_index = malloc(sizeof(int));
                *send_index = i;                
                pthread_create(&send_threads[i], NULL, &send_thread_routine, (void *) send_index);
            }                                    
               
            // wait for recv thread and send threads;
            int send_end_count = 0;
            int recv_end_count = 0;   
            int send_end = 0;
            int recv_end = 0;                 
            int send_count = 0;
            int recv_count = 0;
            int to_send_count = 0;
            int to_recv_count = 0;
            int sockfd;
            int total_send_count = 0;
            time_t last_log_time = 0;     
            int time_to_log = 0;              
            time_t last_report_time = 0;
            while ((send_end_count + recv_end_count < 2 * STORAGE_COUNT) 
            && (recv_running || send_running) && !has_error) {
                total_send_count = 0;
                send_end_count = 0;
                recv_end_count = 0;
                now = time((time_t *)NULL);
                time_to_log = now - last_log_time >= CLIENT_LOG_INTERVAL ? 1 : 0;
                if (time_to_log)
                    last_log_time = now;
                for (i = 0; i < STORAGE_COUNT; i ++) {
                    send_end = 0;
                    recv_end = 0;                    
                    pthread_mutex_lock(&storages[i].send_buffer.status_mutex);
                    to_send_count = storages[i].send_buffer.to_send;
                    send_count = storages[i].send_buffer.send_count;
                    to_recv_count = storages[i].range.count;
                    recv_count = storages[i].send_buffer.recv_count;                    
                    sockfd = storages[i].sockfd;
                    total_send_count += storages[i].send_buffer.send_count;
                    if (storages[i].sockfd < 0 || recv_count >= to_recv_count) {
                        if (storages[i].sockfd < 0) {
                            has_error = 1;
                        }
                        recv_end = 1;
                        recv_end_count ++;
                        if (storages[i].sockfd < 0 || send_count >= to_send_count) {
                            if (storages[i].sockfd < 0) {
                                has_error = 1;
                            }                            
                            send_end = 1;
                            send_end_count ++;
                        }
                    }
                    pthread_mutex_unlock(&storages[i].send_buffer.status_mutex);
                    if (time_to_log) {
                        printf("*****************************[%s]\n", storages[i].ip_address);
                        if (recv_end) {
                            printf("[%s] <<--- [%s] sockfd: %d, +++++ recv complete! to recv count: %d\n", get_string_from_time(now), 
                                storages[i].ip_address, sockfd, to_recv_count);                        
                        } else {
                            printf("[%s] <<--- [%s] sockfd: %d, to recv count: %d, recv count: %d\n", get_string_from_time(now), 
                                storages[i].ip_address, sockfd, to_recv_count, recv_count);                                                
                        }
                        if (send_end) {
                            printf("[%s] --->> [%s] sockfd: %d, +++++ send complete! to send count: %d\n", get_string_from_time(now), 
                                storages[i].ip_address, sockfd, to_send_count);                        
                        } else {
                            printf("[%s] --->> [%s] sockfd: %d, to send count: %d, send count: %d\n", get_string_from_time(now), 
                                storages[i].ip_address, sockfd, to_send_count, send_count);                                                
                        }
                    }
                }

                // report to server
                if (now - last_report_time >= CLIENT_REPORT_INTERVAL) {
                    progress.state = 1;
                    progress.count = total_send_count;
                    int ret = send_data_block(server_socket, &progress, sizeof(TProgress), 0);
                    if (ret <= 0) {
                        // server socket error
                        recv_running = 0;
                        send_running = 0;
                        printf("Server socket error!\n"); 
                    }
                }                
                sleep(1);
            }

            //set end status
            recv_running = 0;
            send_running = 0;
            // wait for threads;
            for (i = 0; i < CLIENT_RECV_THREAD_COUNT; i ++) {
                pthread_join(recv_threads[i], NULL);
            }                             
            for (i = 0; i < CLIENT_SEND_THREAD_COUNT; i ++) {
                pthread_join(send_threads[i], NULL);
            }     
            
            // close_sockets
            for (i = 0; i < STORAGE_COUNT; i ++) {
                close(storages[i].sockfd);
                storages[i].sockfd = -1;
                bzero(&storages[i].range, sizeof(storages[i].range));
                bzero(&storages[i].send_buffer, sizeof(storages[i].send_buffer));
            }       
            printf("process complete! cost: >>> %ld seconds <<<\n", (time((time_t *)NULL) - begin_time));
                
            // send progress
            TProgress progress;
            progress.state = has_error ? 2 : 1;
            progress.count = index_head.count;
            send_data_block(server_socket, &progress, sizeof(TProgress), 0);
            // close server socket
            close(server_socket);
            server_socket = -1;
            // wait for 3 second
            sleep(3);
        } else {
            time_t now = time((time_t *)NULL);
            printf("[%s] connecting to server error, errno = %d, errmsg = %s\n", get_string_from_time(now), errno, strerror(errno));
            fflush(stdout);
            fflush(stderr);            
            if (server_socket > -1) {
                TProgress progress;
                progress.state = 2;
                progress.count = 0;
                send_data_block(server_socket, &progress, sizeof(TProgress), 0);
                close(server_socket);
                server_socket = -1;
            }
            // wait for 3 second
            sleep(3);
        }
        // clear buffer
        if (NULL != index_items) {
            free (index_items);   
            index_items = NULL;             
        }             
    }
    
    destory_client();
    
    return 0;
}
