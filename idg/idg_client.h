/*
 *  idg_client.h
 *  IDG test client
 *  Created on: Nov 4, 2013
 *      Author: qiuxi.zhang
 */

#ifndef _IDG_CLIENT_H_
#define _IDG_CLIENT_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "idg_common.h"

/* client */
typedef struct range {
    int from;
    int thru;    
    int count;
} range_t;

/* storage */
typedef struct storage {
    int sockfd;
    char ip_address[INET6_ADDRSTRLEN];
    int count_down;
    int test_send_count;
    int test_recv_count;
    int serial_no;
    range_t range;    
    TSendBuffer send_buffer;
} storage_t;


#endif