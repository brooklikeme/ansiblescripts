#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "test_common.h"

int main( int argc, char *argv[] )
{
    time_t now;
    TReportHead head;
    time_t last_debug_time = time((time_t *)NULL);    
    char send_buffer[SOCK_PACKET_SIZE];
    char ip_address[15];
    int send_count;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [ip address] [send count]\n", argv[0]);
        return 1;
    }
    
    strcpy(ip_address, argv[1]);
    send_count = atoi(argv[2]);
    
    int sock;
    struct sockaddr_in ssock;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    ssock.sin_family = AF_INET;
    ssock.sin_addr.s_addr = inet_addr(argv[1]);
    ssock.sin_port = htons(SERVER_PORT);
    if (connect(sock, (struct sockaddr *)&ssock, sizeof(ssock)) < 0) {
        perror("ERROR on connect");
        exit(1);        
    }
    
    printf("---------------------------------------------------------------------\n");    
    printf("Connection success to %s on port %d send count = %d \n", ip_address, SERVER_PORT, send_count);       
    
    // send head data
    head.count = send_count;
    if (send_data(sock, &head, sizeof(TReportHead), 0) <= 0) {
        close(sock);
        return -1;
    } 
    
    send_count = 0;
    int curr_send_count = 0;
    while (send_count < head.count) {
        if (head.count - send_count > REPORT_MTU_COUNT) {
            curr_send_count = REPORT_MTU_COUNT;
        } else {
            curr_send_count = head.count - send_count;
        }
        if (send_data(sock, send_buffer, sizeof(TReportItem) * curr_send_count, 1) <= 0) {
            close(sock);
            return -1;
        } 
        now = time((time_t *)NULL);        
        send_count += curr_send_count;
        if (now > last_debug_time) {
            printf("[%s] --->> [%s] send count = %d\n", get_string_from_time(now), ip_address, send_count);
            last_debug_time = now;            
        }        
    }
    
    printf("[%s] --->> [%s] send complete!\n", get_string_from_time(now), ip_address);    
    close(sock);    
    return 0;
}
