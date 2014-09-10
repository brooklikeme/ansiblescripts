/*
 *  idg_storage.h
 *  IDG test storage
 *  Created on: Nov 4, 2013
 *      Author: qiuxi.zhang
 */

#ifndef _IDG_STORAGE_H_
#define _IDG_STORAGE_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 32

/* send buffer */
typedef struct send_buffer {
    int send_index;
    int send_count;
} send_buffer_t;
    
/* client */
typedef struct client {
    int sockfd;
    char ip_address[INET6_ADDRSTRLEN];
    int serial_no;
    TSendBuffer send_buffer;    
} client_t;

#endif