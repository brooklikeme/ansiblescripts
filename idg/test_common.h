#ifndef _TEST_COMMON_H_
#define _TEST_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <sys/types.h>
#include <sys/shm.h>
#include <netinet/in.h>

#define REPORT_MTU_COUNT 1024
#define SOCK_PACKET_SIZE 30240

#define SERVER_PORT 5001

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

char *get_string_from_time(time_t time);

// send socket data
long long send_data(int sockfd, void * data, long long to_send, int reuse);
// recv socket data
long long recv_data(int sockfd, void * data, long long to_read, int reuse);

#endif