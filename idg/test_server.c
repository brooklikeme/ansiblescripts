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

int doprocessing(int sock, char *ip_address)
{
    time_t now;
    time_t last_debug_time = time((time_t *)NULL);
    TReportHead head;
    char recv_buffer[SOCK_PACKET_SIZE];
    
    if (recv_data(sock, &head, sizeof(TReportHead), 0) <= 0) {
        close(sock);
        return -1;
    } 
    // printf
    printf("To recv count = %d\n", head.count);
    
    int recv_count = 0;
    int curr_recv_count = 0;
    // loop get 
    while (recv_count < head.count) {
        if (head.count - recv_count > REPORT_MTU_COUNT) {
            curr_recv_count = REPORT_MTU_COUNT;
        } else {
            curr_recv_count = head.count - recv_count;
        }
        if (recv_data(sock, recv_buffer, sizeof(TReportItem) * curr_recv_count, 1) <= 0) {
            close(sock);
            return -1;
        } 
        now = time((time_t *)NULL);        
        recv_count += curr_recv_count;
        if (now > last_debug_time) {
            printf("[%s] <<--- [%s] recv count = %d\n", get_string_from_time(now), ip_address, recv_count);            
            last_debug_time = now;            
        }
    }
    
    printf("[%s] <<--- [%s] recv complete!\n", get_string_from_time(now), ip_address);
    return 0;
}

int main( int argc, char *argv[] )
{
    pid_t pid;
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(1);
    }
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = SERVER_PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
 
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                          sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
         exit(1);
    }
    /* Now start listening for the clients, here 
     * process will go in sleep mode and will wait 
     * for the incoming connection
     */
    listen(sockfd, 5);
    socklen_t size = sizeof(struct sockaddr_in);      
    
    while (1)  {
        char ip_address[15];
        newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, &size);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }
        printf("---------------------------------------------------------------------\n");    
        printf("New connection from %s on port %d\n",
            inet_ntoa(cli_addr.sin_addr), htons(cli_addr.sin_port));   
        strcpy(ip_address, inet_ntoa(cli_addr.sin_addr));
        /* Create child process */
        pid = fork();
        if (pid < 0)
        {
            perror("ERROR on fork");
            exit(1);
        }
        if (pid == 0)  
        {
            /* This is the client process */
            close(sockfd);
            if (doprocessing(newsockfd, ip_address) < 0) {
                exit(1);
            }
            exit(0);
        }
        else
        {
            close(newsockfd);
        }
    } /* end of while */
}