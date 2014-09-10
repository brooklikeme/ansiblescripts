/*
 *  idg_common.c
 *  IDG test common
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

#include "idg_common.h"

char time_string[50];
char time_format_string[50];
char traffic_string[50];

int is_non_blocking(int sock_fd) {
    int flags = fcntl(sock_fd, F_GETFL, 0);
    return flags & O_NONBLOCK;
}
//
char *get_string_from_time(time_t time)
{
    struct tm *tb = localtime(&time);
    char fmt[] = "%Y-%m-%d %H:%M:%S";
    strftime(time_format_string, sizeof(time_format_string), fmt, tb);
    return time_format_string;
}

//
char * get_time_string(int duration)
{
    int second, minute, hour;
    second = duration;
    hour = duration / 3600;
    second = duration % 3600;
    minute = second / 60;
    second = second % 60;
    bzero(time_string, sizeof(time_string));    
    sprintf(time_string, "%02d:%02d:%02d", hour, minute, second);
    return time_string;
}

//
char * get_traffic_string(unsigned long long traffic)
{
    unsigned long long int_kilo = 1024;
    unsigned long long int_mega = 1048576;
    unsigned long long int_giga = 1073741824;
    unsigned long long int_tera = 1099511627776;
    bzero(traffic_string, sizeof(traffic_string));
    if (traffic >= int_tera) {
        sprintf(traffic_string, "%6.2f TB", traffic * 1.0 / int_tera);
    } else if (traffic >= int_giga) {
        sprintf(traffic_string, "%6.2f GB", traffic * 1.0 / int_giga);
    } else if (traffic >= int_mega) {
        sprintf(traffic_string, "%6.2f MB", traffic * 1.0 / int_mega);
    } else if (traffic >= int_kilo) {
        sprintf(traffic_string, "%6.2f KB", traffic * 1.0 / int_kilo);
    } else {
        sprintf(traffic_string, "%7llu B", traffic);
    }
    return traffic_string;    
}

// recv socket data
long long recv_data_block(int sockfd, void * data, long long to_read, int reuse)
{
    int bytes  = 0;
    long long n_read = 0;
    long long read_len = 0;
	while (n_read < to_read) {
        read_len = to_read - n_read < SOCK_PACKET_SIZE 
            ? to_read - n_read : SOCK_PACKET_SIZE;
		bytes = recv(sockfd, (void *) ((reuse ? 0 : n_read) + (long long)data), read_len, 0);
        if (bytes < 0) {
            perror("read");
            return -1;
        }
        if (bytes == 0) {
            return 0;
        }        
		n_read += bytes;
	}   
    return n_read; 
}

// send socket data
long long send_data_block(int sockfd, void * data, long long to_send, int reuse)
{    
    // send head data
    int bytes  = 0;
    long long n_send = 0;
    long long send_len = 0;
    int loop_count = 0;
	while (n_send < to_send) {
        loop_count ++;
        send_len = to_send - n_send < SOCK_PACKET_SIZE 
            ? to_send - n_send : SOCK_PACKET_SIZE;
		bytes = send(sockfd, (void *) ((reuse ? 0 : n_send) + (long long)data), send_len, 0);
        if (bytes < 0) {
            // DEBUG
            perror("write");
            return -1;
        }
        if (bytes == 0) {
            return 0;
        }
		n_send += bytes;
	}       
    return n_send; 
}

// recv socket data
long long recv_data_non_block(int sockfd, void * data, long long to_read, int reuse, int timeout, int debug)
{
    int bytes  = 0;
    long long n_read = 0;
    long long read_len = 0;    
    int ret, max_sd = 0;
    fd_set recv_set;
    struct timeval process_time, select_time;
    long long begin_time, end_time, select_begin_time, select_end_time, max_select_time = 0;
    if (debug) {        
        gettimeofday(&process_time, NULL);        
        begin_time = process_time.tv_sec * 1000000 + process_time.tv_usec;        
    }
    
    struct timeval timeout_read;  
    timeout_read.tv_sec  = timeout;
    timeout_read.tv_usec = 0;
    int loop_count = 0, max_index = 0;
    while (n_read < to_read) {
        loop_count ++;                        
        // Initialize fd        
        FD_ZERO(&recv_set);
        FD_SET(sockfd, &recv_set);
        max_sd = sockfd;        
        if (debug) {
            gettimeofday(&select_time, NULL);
            select_begin_time = select_time.tv_sec * 1000000 + select_time.tv_usec;
        }                        
        ret = select(max_sd + 1, &recv_set, NULL, NULL, &timeout_read);
        if (debug) {
            gettimeofday(&select_time, NULL);
            select_end_time = select_time.tv_sec * 1000000 + select_time.tv_usec;
            max_select_time = select_end_time - select_begin_time > max_select_time ? select_end_time - select_begin_time : max_select_time;            
            max_index = loop_count - 1;
        }                                
        if (ret < 0) {
            perror("select()");
            printf("select error! sock = %d, errno = %d, errmsg = %s\n", sockfd, errno, strerror(errno));
            return -1;
        } else if (ret == 0) {
            printf("recv_data_non_block select timeout, threshold: %d sec, max_select_time: %lld, max_index: %d, to_read = %lld, n_read = %lld\n", 
                timeout, max_select_time, max_index, to_read, n_read);
            return -1;
        } else {
            while (n_read < to_read) {
                read_len = to_read - n_read < SOCK_PACKET_SIZE 
                    ? to_read - n_read : SOCK_PACKET_SIZE;
        		bytes = recv(sockfd, (void *) ((reuse ? 0 : n_read) + (long long)data), read_len, 0);
                if (bytes < 0) {
                    if (errno != EWOULDBLOCK) {
                        perror("recv()");
                        printf("recv error! sock = %d, errno = %d, errmsg = %s\n", sockfd, errno, strerror(errno));                        
                        return -1;                        
                    } else {
                        // nothing to read
                        break;                        
                    }
                } else if (bytes == 0) {
                    return 0;
                } else {
                    n_read += bytes;                    
                }        
            }
        } 
    }
    if (debug) {
        gettimeofday(&process_time, NULL);   
        time_t now = time((time_t *)NULL);         
        end_time = process_time.tv_sec * 1000000 + process_time.tv_usec;
        if (end_time - begin_time > 2000000)
            printf("**** [%s] recv_data_non_block() <<--- to_read = %lld cost %lld us max_select_time = %lld us loop_count = %d, max_index = %d, sockfd = %d\n", 
                get_string_from_time(now), to_read, end_time - begin_time, max_select_time, loop_count, max_index, sockfd);         
    }
    
    return n_read; 
}

// send socket data
long long send_data_non_block(int sockfd, void * data, long long to_send, int reuse, int timeout, int debug)
{    
    // send head data
    int bytes  = 0;
    long long n_send = 0;
    long long send_len = 0;
    int ret, max_sd = 0;
    fd_set send_set;
    struct timeval timeout_read;  
    timeout_read.tv_sec  = timeout;
    timeout_read.tv_usec = 0;
    
    struct timeval process_time, select_time;
    long long begin_time, end_time, select_begin_time, select_end_time, max_select_time = 0;
    if (debug) {        
        gettimeofday(&process_time, NULL);        
        begin_time = process_time.tv_sec * 1000000 + process_time.tv_usec;        
    }
    int loop_count = 0, max_index = 0;
    while (n_send < to_send) {
        loop_count ++;
        // Initialize fd
        FD_ZERO(&send_set);
        FD_SET(sockfd, &send_set);
        max_sd = sockfd;
        if (debug) {
            gettimeofday(&select_time, NULL);
            select_begin_time = select_time.tv_sec * 1000000 + select_time.tv_usec;
        }                        
        
        ret = select(max_sd + 1, NULL, &send_set, NULL, &timeout_read);
        if (debug) {
            gettimeofday(&select_time, NULL);
            select_end_time = select_time.tv_sec * 1000000 + select_time.tv_usec;
            max_select_time = select_end_time - select_begin_time > max_select_time ? select_end_time - select_begin_time : max_select_time;            
            max_index = loop_count - 1;
        }                                        
        if (ret < 0) {
            perror("select()");
            printf("select() error! sock = %d, errno = %d, errmsg = %s\n", sockfd, errno, strerror(errno));                                    
            return -1;
        } else if (ret == 0) {
            printf("send_data_non_block() select timeout, threshold: %d sec, max_select_time: %lld, max_index: %d, to_send = %lld, n_send = %lld\n", 
                timeout, max_select_time, max_index, to_send, n_send);
            return -1;
        } else {
        	while (n_send < to_send) {
                send_len = to_send - n_send < SOCK_PACKET_SIZE 
                    ? to_send - n_send : SOCK_PACKET_SIZE;
        		bytes = send(sockfd, (void *) ((reuse ? 0 : n_send) + (long long)data), send_len, 0);
                if (bytes < 0) {
                    if (errno != EWOULDBLOCK) {
                        perror("send()");
                        printf("send error! sock = %d, errno = %d, errmsg = %s\n", sockfd, errno, strerror(errno));                                                
                        return -1;                        
                    } else {
                        // nothing to read
                        break;                        
                    }
                } else if (bytes == 0) {
                    return 0;
                } else {
                    n_send += bytes;                    
                }        
        	}                   
        }
    }
    if (debug) {
        gettimeofday(&process_time, NULL);   
        time_t now = time((time_t *)NULL);         
        end_time = process_time.tv_sec * 1000000 + process_time.tv_usec;
        if (end_time - begin_time > 2000000)
            printf("**** [%s] send_data_non_block() <<--- to_send = %lld cost %lld us max_select_time = %lld us loop_count = %d, max_index = %d, sockfd = %d\n", 
                get_string_from_time(now), to_send, end_time - begin_time, max_select_time, loop_count, max_index, sockfd);         
    }

    return n_send; 
}

//
int recv_check_and_lock(TSendBuffer *buffer)
{
    int ret = 0;
    pthread_mutex_lock(&buffer->socket_mutex);   
    if (!buffer->recving) {
        buffer->recving = 1;
        ret = 1;
    }
    pthread_mutex_unlock(&buffer->socket_mutex);        
    return ret;
}

//
int send_check_and_lock(TSendBuffer *buffer)
{
    int ret = 0;
    pthread_mutex_lock(&buffer->socket_mutex);   
    if (!buffer->sending) {
        buffer->sending = 1;
        ret = 1;
    } 
    pthread_mutex_unlock(&buffer->socket_mutex);   
    return ret;
}

//
void recv_unlock(TSendBuffer *buffer)
{
    pthread_mutex_lock(&buffer->socket_mutex);    
    buffer->recving = 0;
    pthread_mutex_unlock(&buffer->socket_mutex);    
}

//
void send_unlock(TSendBuffer *buffer)
{
    pthread_mutex_lock(&buffer->socket_mutex);    
    buffer->sending = 0;
    pthread_mutex_unlock(&buffer->socket_mutex);     
}

//
void add_send_head(TSendBuffer *buffer, int index, int count)
{
    pthread_mutex_lock(&buffer->status_mutex);    
    buffer->heads[index].type = 0;
    buffer->heads[index].index = index;    
    buffer->heads[index].count = count;
    pthread_mutex_unlock(&buffer->status_mutex);  
    printf("DEBUG add_send_head() index = %d, count = %d\n", index, count);
}

// 
PReportHead fetch_send_head(TSendBuffer *buffer)
{
    int i;
    PReportHead report_head = NULL;
    pthread_mutex_lock(&buffer->status_mutex);
    for (i = 0; i < STORAGE_COUNT; i++) {
        if (buffer->heads[i].count > 0) {
            report_head = &buffer->heads[i];
            break;
        }
    }
    pthread_mutex_unlock(&buffer->status_mutex);
    return report_head;
}

//
void add_send_count(TSendBuffer *buffer, int count)
{
    pthread_mutex_lock(&buffer->status_mutex);
    // change status
    buffer->buffer_count += count;
    pthread_mutex_unlock(&buffer->status_mutex);
}

// 
int fetch_send_count(TSendBuffer *buffer)
{
    int send_count = 0;
    pthread_mutex_lock(&buffer->status_mutex);    
    if (buffer->buffer_count > REPORT_MTU_COUNT) {
        send_count = REPORT_MTU_COUNT;
        buffer->buffer_count -= REPORT_MTU_COUNT;
    } else {
        send_count = buffer->buffer_count;
        buffer->buffer_count = 0;
    }    
    pthread_mutex_unlock(&buffer->status_mutex);   
    return send_count;
}

//
void add_recv_count(TSendBuffer *buffer, int count)
{
    pthread_mutex_lock(&buffer->status_mutex);
    // change status
    buffer->recv_count += count;    
    pthread_mutex_unlock(&buffer->status_mutex);
}

//
int check_recv_status(int sockfd)
{
    int ret, max_sd = 0;
    fd_set recv_set;
    // Initialize the master fd_set
    FD_ZERO(&recv_set);
    FD_SET(sockfd, &recv_set);
    max_sd = sockfd;

    struct timeval timeout_read;  
    timeout_read.tv_sec  = 0;
    timeout_read.tv_usec = 0;
    ret = select(max_sd + 1, &recv_set, NULL, NULL, &timeout_read);
    if (ret < 0) {
        perror("check_recv_status() ERROR : select() failed!");        
    }
        
    return ret;
}

