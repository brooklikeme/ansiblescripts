/*
 *  idg_common.h
 *  IDG test common head file
 *  Created on: Nov 4, 2013
 *      Author: qiuxi.zhang
 */

#ifndef _IDG_COMMON_H_
#define _IDG_COMMON_H_

#define DEBUG

// common parameters
#define INDEX_FILE_PR     "idg_index.dat"
#define FEEDBACK_INTERVAL 5  
#define PROGRESS_INTERVAL 10

#define CLIENT_SEND_THREAD_COUNT 1
#define CLIENT_RECV_THREAD_COUNT 2

#define STORAGE_SEND_THREAD_COUNT 1
#define STORAGE_RECV_THREAD_COUNT 2     

#define CONNECT_TIMEOUT 60
#define RECV_TIMEOUT 3600
#define SEND_TIMEOUT 3600

#define CLIENT_LOG_INTERVAL 1
#define CLIENT_REPORT_INTERVAL 5

// address parameters
#define SERVER_PORT       8889
#define STORAGE_PORT      9999
#define SERVER_ADDRESS    "10.5.1.6"
#define STORAGE_ADDRESS   "10.5.3.1,10.5.3.4"
#define CLIENT_COUNT      3
#define STORAGE_COUNT     2

// optimize parameters
#define SND_BUFFER_SIZE   0
#define RCV_BUFFER_SIZE   0
#define SOCK_PACKET_SIZE  65535//65535
#define QUEUE_SIZE        5
#define PARALLEL_SIZE     4
#define REPORT_MTU_COUNT  1024

// index head
typedef struct INDEX_HEAD {
    unsigned int count;
    unsigned int is_test;
} TIndexHead, * PIndexHead;

// index item
typedef struct INDEX_ITEM {
    unsigned int dim1;
    unsigned int dim2;
    unsigned int dim3;    
    unsigned int index1;
    unsigned int index2;    
} TIndexItem, * PIndexItem;

// index head
typedef struct PROTGRESS {
    unsigned int state; //0: normal, 1:error
    unsigned int count;
} TProgress;

// report head
typedef struct REPORT_HEAD {
	unsigned int type; // 0:read, 1:write
	unsigned int count;
    unsigned int index;
    unsigned int serial_no;
} TReportHead, * PReportHead;

// report item
typedef struct REPORT_ITEM {
    char head[240];
    char body[8000];
} TReportItem, * PReportItem;

// buffer item
typedef struct SEND_BUFFER {
    TReportHead heads[STORAGE_COUNT];    
    int buffer_count;
    int to_send;
    int recv_count;
    int send_count;
    int sending;
    int recving;
    pthread_mutex_t socket_mutex;
    pthread_mutex_t status_mutex;
} TSendBuffer, * PSendBuffer;

long long recv_data_block(int sockfd, void * data, long long to_read, int reuse);

long long send_data_block(int sockfd, void * data, long long to_send, int reuse);

long long recv_data_non_block(int sockfd, void * data, long long to_read, int reuse, int stringByDeletingPathExtension, int debug);

long long send_data_non_block(int sockfd, void * data, long long to_send, int reuse, int timeout, int debug);

char * get_time_string(int duration);
char * get_traffic_string(unsigned long long traffic);
char * get_string_from_time(time_t time);

//
int recv_check_and_lock(TSendBuffer *buffer);
//
int send_check_and_lock(TSendBuffer *buffer);
//
void recv_unlock(TSendBuffer *buffer);
//
void send_unlock(TSendBuffer *buffer);
//
void add_send_head(TSendBuffer *buffer, int index, int count);
// 
PReportHead fetch_send_head(TSendBuffer *buffer);
//
void add_send_count(TSendBuffer *buffer, int count);
//
void add_recv_count(TSendBuffer *buffer, int count);
// 
int fetch_send_count(TSendBuffer *buffer);
//
int check_recv_status(int sockfd);
//
int is_non_blocking(int sock_fd);

#endif