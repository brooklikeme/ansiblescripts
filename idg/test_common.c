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
#include <fcntl.h>
#include <sys/ioctl.h>

#include "test_common.h"

char time_format_string[50];

char *get_string_from_time(time_t time)
{
    struct tm *tb = localtime(&time);
    char fmt[] = "%Y-%m-%d %H:%M:%S";
    strftime(time_format_string, sizeof(time_format_string), fmt, tb);
    return time_format_string;
}

// send socket data
long long send_data(int sockfd, void * data, long long to_send, int reuse)
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
long long recv_data(int sockfd, void * data, long long to_read, int reuse)
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

