/*
 *  idg_server.c
 *  IDG test server
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
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "idg_common.h"
#include "idg_server.h"

// buffer
client_t clients[CLIENT_COUNT];
int socket_listener;
int index_count = 0;
int sending_count = CLIENT_COUNT;
pthread_mutex_t sending_mutex;
PIndexItem index_items;
int is_test = 0;

//
void init_server()
{
    int i;
    // init client list
    bzero(clients, sizeof(clients));
    for (i = 0; i < CLIENT_COUNT; i ++) {
        clients[i].sockfd = -1;
        pthread_mutex_init(&clients[i].progress_mutex, NULL);    
    }
    pthread_mutex_init(&sending_mutex, NULL);    
}

//
int init_listener()
{
    socket_listener = -1;
    struct addrinfo flags;
    struct addrinfo *host_info;

    char port_str[10];
    bzero(port_str, sizeof(port_str));
    sprintf(port_str, "%d", SERVER_PORT);    
    
    memset(&flags, 0, sizeof(flags));
    flags.ai_family = AF_UNSPEC; 
    flags.ai_socktype = SOCK_STREAM;
    flags.ai_flags = AI_PASSIVE;
    
    char ip_address[INET6_ADDRSTRLEN] = SERVER_ADDRESS;

    if (getaddrinfo(ip_address, port_str, &flags, &host_info) < 0) {
        perror("Couldn't read host info for socket start");
        return -1;
    }
    
    socket_listener = socket(host_info->ai_family, host_info->ai_socktype, 
        host_info->ai_protocol);    
    if (socket_listener < 0) {
        perror("Error opening socket");
        return -1;
    }
    
    int yes = 1, on = 1;
    
    if (setsockopt(socket_listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) < 0) {                     
        perror("Error setsockopt");
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
int wait_for_clients() 
{
    int i;
    int client_number = 0;   
    struct timeval timeout_read;      
    int ret, max_sd = 0, new_sd = -1;
    fd_set recv_set;    
    
    while (client_number < CLIENT_COUNT) {
        FD_ZERO(&recv_set);
        FD_SET(socket_listener, &recv_set);
        max_sd = socket_listener;
        timeout_read.tv_sec  = 10;
        timeout_read.tv_usec = 0;        
        ret = select(max_sd + 1, &recv_set, NULL, NULL, &timeout_read);
        if (ret < 0) {
            perror("select() failed");
            close(socket_listener);
            return -1;
        } else if (ret == 0) {
            printf("select() timeout\n");  
            close(socket_listener);
            return -1;
        } else if (ret > 0) {
            if (FD_ISSET(socket_listener, &recv_set)) {
                struct sockaddr_in their_addr;
                socklen_t size = sizeof(struct sockaddr_in);
                new_sd = accept(socket_listener, (struct sockaddr*)&their_addr, &size);
                if (new_sd < 0) {
                    if (errno != EWOULDBLOCK) {
                        perror("Error on accept");
                        close(socket_listener);
                        return -1;
                    } else {
                        printf("accept() in EWOULDBLOCK!\n");
                        break;
                    }                   
                } else {
                    printf("New connection from %s on port %d\n",
                        inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port));
                    fflush(stderr);                                                               
                    fflush(stdout); 
                    for (i = 0; i < CLIENT_COUNT; i ++) {
                        if (clients[i].sockfd == -1) {
                            clients[i].sockfd = new_sd;
                            strcpy(clients[i].ip_address, inet_ntoa(their_addr.sin_addr));    
                            clients[i].connect_time = time((time_t *)NULL);                
                            client_number ++;
                            break;            
                        }
                    }                                                                                                                                       
                }
            }
        }
        fflush(stdout);
        fflush(stderr);        
        // sleep a while          
        usleep(1000 * 1);
    }
    // close listen socket
    close(socket_listener);
    
    return 0;
}

// 
int gen_index_data()
{
    // load index file to memory
    int i;
    FILE *fp;
    TIndexItem index_item;
    char index_file_name[50];
    sprintf(index_file_name, "%s.%d", INDEX_FILE_PR, index_count); 
        
    fp = fopen(index_file_name, "wb+");
    if (NULL == fp) {
        perror("Error open index file");
        return -1;
    }    
        
    // write index count
    if (fwrite(&index_count, sizeof(int), 1, fp) < 1) {
        printf("Error writing index count.\n");
        if (fclose(fp) != 0) {
            perror("Error close index file");
            return -1;
        }        
        return -1;
    }    

    printf("Writing index data to %s..., data number: %d, data size: %ld Bytes\n", 
        index_file_name, index_count, index_count * sizeof(TIndexItem));
    fflush(stdout);        
        
    // gen index data
	srandom(time(NULL));

    int remain = 1000000;
    float divisor = index_count * 1.0 / STORAGE_COUNT;
    for (i = 0; i < index_count; i ++) {
        index_item.dim1 = random() % remain;
        index_item.dim2 = random() % remain;
        index_item.dim3 = random() % remain;
        index_item.index2 = 0;        
        index_item.index1 = (long long)(i * 1.0 / divisor);
        fwrite(&index_item, sizeof(TIndexItem), 1, fp);
    }
    
    printf("Writing index data complete!\n");
    fflush(stdout);    
        
    // close file description
    if (fclose(fp) != 0) {
        perror("Error close index file");
        return -1;
    }
    
    return 0;
}

//
int read_index_data()
{
    // load index file to memory
    FILE *fp;
            
    char index_file_name[50];
    sprintf(index_file_name, "%s.%d", INDEX_FILE_PR, index_count); 
            
    fp = fopen(index_file_name, "rb");
    if (NULL == fp) {
        perror("Error open index file");
        return -1;
    }
    
    // read index count
    if (fread(&index_count, sizeof(int), 1, fp) < 1) {
        printf("Error reading index count.\n");
        if (fclose(fp) != 0) {
            perror("Error close index file");
            return -1;
        }        
        return -1;
    }
    
    printf("Reading index data... number: %d, size: %s\n", 
        index_count, get_traffic_string(index_count * sizeof(TReportItem)));
    fflush(stdout);        
    
    // loading
    index_items = malloc(index_count * sizeof(TIndexItem));
    int index = 0;
    while (index < index_count && !feof(fp)) {
        fread(index_items + index, sizeof(TIndexItem), 1, fp);
        index ++;
    }    
        
    // close file description
    if (fclose(fp) != 0) {
        perror("Error close index file");
        return -1;
    }
    printf("Reading index data complete!\n");
    fflush(stdout);    

    return 0;
}
    
//
int index_item_comp(const void *item1, const void *item2)
{
    return ((PIndexItem)item1)->dim1 == ((PIndexItem)item2)->dim1 ?
        ((PIndexItem)item1)->dim2 - ((PIndexItem)item2)->dim2 
        : ((PIndexItem)item1)->dim1 - ((PIndexItem)item2)->dim1;
}
    
//
int sort_index_data()
{
    int i;
    // do quicksort
    printf("Sorting and assigning index data...\n");
    fflush(stdout);    
    time_t begin_time = time((time_t *)NULL);
        
    qsort(index_items, index_count, sizeof(TIndexItem), index_item_comp);
    
    printf("Quick sort complete!\n");
    fflush(stdout);    
            
    int old_index = 0, new_index = 0;
    float client_divisor  = index_count * 1.0 / CLIENT_COUNT;    
    float storage_divisor = index_count * 1.0 / STORAGE_COUNT;
    for (i = 0; i < index_count; i ++) {
        index_items[i].index2 = (long long)(i * 1.0 / storage_divisor);        
        new_index = (long long)(i * 1.0 / client_divisor);
        if (new_index != old_index) {
            clients[old_index].thru_index = i - 1;
            clients[new_index].from_index = i;
            old_index = new_index;
        }
        if (i == index_count - 1) {
            clients[old_index].thru_index = i;
        }
    }
    
    /*
    for (i = 0; i < CLIENT_COUNT; i ++) {
        printf("DEBUG client: %d, from_index = %d, thru_index = %d\n", 
            i, clients[i].from_index, clients[i].thru_index);
    }*/
    
    time_t now = time((time_t *)NULL);
    printf("Sorting and assigning index data complete! cost: >>> %ld seconds <<<\n", now - begin_time);
    fflush(stdout);
    
    return 0;
}

//
void* client_thread(void *arg)
{
    int *pindex = (int *) arg;
    int client_index = *pindex;
    free (pindex);
    client_t *client = &clients[client_index];
    int sockfd = client->sockfd;
    
    long long n_send = 0, n_read  = 0;
	TIndexHead head;
    head.count = client->thru_index - client->from_index + 1;
    head.is_test = is_test;

    // send head data
    n_send = send_data_block(sockfd, &head, sizeof(head), 0);
    if (n_send < 0) {
        goto socket_error;
    }
    
    // send items data
    n_send = send_data_block(sockfd, 
        index_items + client->from_index, sizeof(TIndexItem) * head.count, 0);
    if (n_send < 0) {
        goto socket_error;
    }            

    // wait for storage conn initialization
    TProgress progress;
    n_read = recv_data_block(sockfd, &progress, sizeof(TProgress), 0);
    if (n_read <= 0) {
        goto socket_error;
    }    

    // reduce index transfer count
    pthread_mutex_lock(&sending_mutex);
    sending_count --;
    if (sending_count == 0) {
        // free index items buffer
        free (index_items);
    }
    pthread_mutex_unlock(&sending_mutex);
    
    printf("Client: %s initialize complete\n", client->ip_address);
    fflush(stdout);
    
    // wait for other client
    int all_sending_over = 0;
    while (!all_sending_over) {
        pthread_mutex_lock(&sending_mutex);   
        all_sending_over = sending_count == 0 ? 1 : 0;     
        pthread_mutex_unlock(&sending_mutex);                
        usleep(1000 * 10);
    }
    
    // send start command
    progress.state = 8888;
    n_send = send_data_block(sockfd, &progress, sizeof(TProgress), 0);
    if (n_read <= 0) {
        goto socket_error;
    }        
    
    // read progress rate    
    while (1) {
        n_read = recv_data_block(sockfd, &progress, sizeof(TProgress), 0);
        if (n_read <= 0) {
            goto normal_return;
        }
        if (progress.state == 2) {
            goto error_return;
        }                
        pthread_mutex_lock(&client->progress_mutex);
        client->progress = progress.count;
        pthread_mutex_unlock(&client->progress_mutex);        
    }   
socket_error:
    pthread_mutex_lock(&client->progress_mutex);
    client->progress = -2;
    pthread_mutex_unlock(&client->progress_mutex);
    close(sockfd);
    pthread_exit(0);
normal_return:
    pthread_mutex_lock(&client->progress_mutex);
    client->progress = -1;
    pthread_mutex_unlock(&client->progress_mutex);
    close(sockfd);
    pthread_exit(0);
error_return:
    pthread_mutex_lock(&client->progress_mutex);
    client->progress = -2;
    pthread_mutex_unlock(&client->progress_mutex);
    close(sockfd);
    pthread_exit(0);
}

//
int wait_for_computing()
{
    printf("Start data transferring and wait for client initialization...\n");
    fflush(stdout);    
    // create threads
    int i;
    for (i = 0; i < CLIENT_COUNT; i ++) {
        int *client_index = malloc(sizeof(int));
        *client_index = i;
        pthread_create(&clients[i].thread, 0, &client_thread, (void *) client_index);
        pthread_detach(clients[i].thread);        
    } 
            
    // wait for client init
    int all_sending_over = 0;
    while (!all_sending_over) {
        pthread_mutex_lock(&sending_mutex);
        all_sending_over = sending_count == 0 ? 1 : 0;
        pthread_mutex_unlock(&sending_mutex);     
        usleep(1000 * 10);   
    }
    printf("All clients initialize complete! processing begins...\n");
    fflush(stdout);

    time_t now;
    time_t begin_time = time((time_t *)NULL);
    time_t last_progress_time = begin_time;    
        
    // wait for thread to complete
    int progress = 0, client_progress;
    int has_error = 0;
    int finish_count = 0;
    while (1) {
        progress = 0;
        finish_count = 0;
        for (i = 0; i < CLIENT_COUNT; i ++) {
            pthread_mutex_lock(&clients[i].progress_mutex);
            client_progress = clients[i].progress;
            pthread_mutex_unlock(&clients[i].progress_mutex);
            if (client_progress == -2) {
                // client return error
                has_error = 1;
            }            
            if (client_progress < 0) {
                finish_count ++;
            }
            if (client_progress == -1)
                progress += clients[i].thru_index - clients[i].from_index + 1;
            else if (client_progress > -1)
                progress += client_progress;            
        }
        if (finish_count >= CLIENT_COUNT) {
            // all finished
            break;
        }
        now = time((time_t *)NULL);
        if (now - last_progress_time >= PROGRESS_INTERVAL) {
            // print progress
            printf("[%s] %s / %3d%% {", get_time_string(now - begin_time), 
                get_traffic_string(progress * sizeof(TReportItem)), (int) (progress * 100.0 / index_count));
            for (i = 0; i < CLIENT_COUNT; i ++) {
                pthread_mutex_lock(&clients[i].progress_mutex);
                client_progress = clients[i].progress == -1 
                    ? clients[i].thru_index - clients[i].from_index + 1 : clients[i].progress;
                if (clients[i].progress == -2) {
                    printf("%11s: ERR", clients[i].ip_address);
                } else {
                    printf("%11s:%3d%%", clients[i].ip_address, 
                        (int) (client_progress * 100.0 / (clients[i].thru_index - clients[i].from_index + 1)));
                }
                if (i < CLIENT_COUNT - 1) {
                    printf(" - ");
                }                
                pthread_mutex_unlock(&clients[i].progress_mutex);                
            }
            printf("}\n");
            last_progress_time = now;
        }
        fflush(stdout);
        sleep(1);
    }
    
    // process end
    now = time((time_t *)NULL);
    if (has_error) {
        printf("Data transferring error! cost: >>> %ld seconds <<<\n",  now - begin_time);        
    } else {
        printf("Data transferring complete! cost: >>> %ld seconds <<<\n",  now - begin_time);                
    }
    printf("\n");        
    fflush(stdout);
    
    return 0;
}

int destory_server()
{
    // free variables
    pthread_mutex_destroy(&sending_mutex);
    
    int i;
    for (i = 0; i < CLIENT_COUNT; i++) {
        pthread_mutex_destroy(&clients[i].progress_mutex);
    }
    
    // close socket
    close(socket_listener);
    
    return 0;
}

// main function
int main(int argc, char *argv[])
{        
	int opt; 
    int is_gen = 0;
	while ((opt = getopt(argc, argv, "tc:n:")) != -1) {
		switch (opt) {
        case 't':
            is_test = 1;
            break;
		case 'c':
            is_gen = 1;
			index_count = atoi(optarg);
			break;
        case 'n':
            index_count = atoi(optarg);
            break;
		default:
			break;
		}
	}
    
    if (is_gen > 0) {
        gen_index_data();
        return 0;
    }
    
    // init data
    init_server();

    // init socket listener
    if (init_listener() < 0) {
        exit(-1);
    }
    
    printf("*********************************************************************\n");    
    printf("idg server is running... (test = %s)\n", is_test ? "true" : "false");
    fflush(stdout);
    
    // setup computing nodes
    if (wait_for_clients() < 0 
        || read_index_data() < 0
        || sort_index_data() < 0
        || wait_for_computing() < 0
        || destory_server() < 0) 
    {
        // close sockets
        int i;
        for (i = 0; i < CLIENT_COUNT; i ++) {
            if (clients[i].sockfd > 0) {
                close(clients[i].sockfd);
                clients[i].sockfd = -1;
            }
        }
        exit(-1);
    }
    
    return 0;
}


