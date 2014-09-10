CC = gcc

all : idg_server idg_client idg_storage test_server test_client
.PHONY : all 

idg_server : idg_server.c idg_server.h idg_common.c idg_common.h
	$(CC) -g -lpthread -o idg_server idg_server.c idg_common.c

idg_client : idg_client.c idg_client.h idg_common.c idg_common.h
	$(CC) -g -lpthread -o idg_client idg_client.c idg_common.c

idg_storage : idg_storage.c idg_storage.h idg_common.c idg_common.h
	$(CC) -g -lpthread -o idg_storage idg_storage.c idg_common.c
	
test_server : test_server.c test_common.c test_common.h
	$(CC) -g -o test_server test_server.c test_common.c

test_client : test_client.c test_common.c test_common.h
	$(CC) -g -o test_client test_client.c test_common.c
	

.PHONY : clean
clean :
	rm -rf idg_server idg_client idg_storage test_server test_client