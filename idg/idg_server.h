/*
 *  idg_server.h
 *  IDG test server
 *  Created on: Nov 4, 2013
 *      Author: qiuxi.zhang
 */

#ifndef _IDG_SERVER_H_
#define _IDG_SERVER_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

/* client */
typedef struct client {
    int sockfd;
    char ip_address[INET6_ADDRSTRLEN];
    time_t connect_time;
    time_t start_time;
    time_t complete_time;
    int from_index;
    int thru_index;
    pthread_t thread;
    int progress;    
    pthread_mutex_t progress_mutex;
} client_t;

#endif