#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CLIENT_COUNT 180
#define MAX_STORAGE_COUNT 180

typedef struct HOST_ITEM {
    char ip_address[100];
    char target_address[100];
} THostItem;

typedef enum {
    lt_none = 0,
    lt_server = 1,
    lt_client = 2,
    lt_storage = 3    
} load_type_t;

THostItem server_host;
int client_count = 0;
THostItem client_hosts[MAX_CLIENT_COUNT];
int storage_count = 0;
THostItem storage_hosts[MAX_STORAGE_COUNT];
char storage_addresses[4096];
    
load_type_t load_type = lt_none;
    
// main function
int main(int argc, char *argv[])
{   
    char line[1024];
    FILE *host_file, *head_file, *temp_file;
    char ip_address[100];
    char target_address_item[100];    
    char target_address[100];

    bzero(client_hosts, sizeof(client_hosts));
    bzero(storage_hosts, sizeof(storage_hosts));
    bzero(storage_addresses, sizeof(storage_addresses));
    
    // init buffer
    host_file = fopen("../tsinghua_host", "r");
    if (host_file) {
        // loop lines
        while (fgets(line, 1024, host_file) != NULL) {
            if (line[0] == '#' || strlen(line) == 0) continue;
            if (strstr(line, "[server]")) {
                // server host begin
                load_type = lt_server;
            } else if (strstr(line, "[client]")) {
                // client hosts begin
                load_type = lt_client;
            } else if (strstr(line, "[storage]")) {
                // storage hosts begin
                load_type = lt_storage;
            } else if (line[0] == '[') {
                load_type = lt_none;
            } else {
                if (load_type == lt_none)
                    continue;
                char *host_config = line;    
                char *host_item;
                if ((host_item = strsep(&host_config, " ")) != NULL) {
                    strcpy(ip_address, host_item);
                    if ((host_item = strsep(&host_config, " ")) != NULL) {
                        strcpy(target_address_item, host_item);
                        host_config = target_address_item;
                        if ((host_item = strsep(&host_config, "=")) != NULL) {
                            if ((host_item = strsep(&host_config, "=")) != NULL) {
                                strcpy(target_address, host_item);
                                //DEBUG
                                if (target_address[strlen(target_address) - 1] == '\r' || target_address[strlen(target_address) - 1] == '\n') {
                                    target_address[strlen(target_address) - 1] = 0;
                                }
                                // add host
                                if (load_type == lt_server) {
                                    strcpy(server_host.ip_address, ip_address);
                                    strcpy(server_host.target_address, target_address);
                                } else if (load_type == lt_client) {
                                    client_count ++;
                                    strcpy(client_hosts[client_count - 1].ip_address, ip_address);
                                    strcpy(client_hosts[client_count - 1].target_address, target_address);                                    
                                } else if (load_type == lt_storage) {
                                    storage_count ++;
                                    strcpy(storage_hosts[storage_count - 1].ip_address, ip_address);
                                    strcpy(storage_hosts[storage_count - 1].target_address, target_address);
                                    if (storage_count > 1) {
                                        strcat(storage_addresses, ",");
                                    }
                                    strcat(storage_addresses, target_address);                                    
                                }
                                printf("DEBUG load_type = %d, ip_address = %s, target_address = %s\n", load_type, ip_address, target_address);
                            }
                        }
                    }
                }
            }
        }
        fclose(host_file);        
    }
    // edit head
    temp_file = fopen("temp.h", "w");
    head_file = fopen("idg_common.h", "r");
    if (head_file && temp_file) {
        // loop lines
        while (fgets(line, 1024, head_file) != NULL) {            
            if (strstr(line, "#define SERVER_ADDRESS") != NULL) {
                fprintf(temp_file, "#define SERVER_ADDRESS    \"%s\"\n", server_host.target_address);
            } else if (strstr(line, "#define STORAGE_ADDRESS") != NULL) {
                fprintf(temp_file, "#define STORAGE_ADDRESS   \"%s\"\n", storage_addresses);                
            } else if (strstr(line, "#define CLIENT_COUNT") != NULL) {
                fprintf(temp_file, "#define CLIENT_COUNT      %d\n", client_count);                                
            } else if (strstr(line, "#define STORAGE_COUNT") != NULL) {
                fprintf(temp_file, "#define STORAGE_COUNT     %d\n", storage_count);                                
            } else {
                fputs(line, temp_file);
            }
        }
    }
    fclose(temp_file);
    fclose(head_file);
    
    //edit head file
    temp_file  = fopen("temp.h", "r");
    head_file = fopen("idg_common.h", "w");
    while (fgets(line, 1024, temp_file))
        fputs(line, head_file);
    fclose(temp_file);
    fclose(head_file);
        
    return 0;
}

