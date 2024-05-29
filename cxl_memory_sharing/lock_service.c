#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hash_kv.h"
#include "log-timestamp.h"
#include "lock.h"

#define LISTEN_BACKLOG (5)
#define ACK_STR "ack ok"

void handle_lock(HashTable* myHashTable, int x, int socket) {
	long int blockid;
        int indicator = x & 3;
	int lock = 0;
	long int value;

	blockid =(long int)((x & 0xfffffffc) >> 2);
        value = getKeyValue(myHashTable, blockid);
	
        if (indicator == WRITE_LOCK) { // 0x2 mean write lock
	  	if (value == -1 || value == 0x0) {
                    LOG("blockedid=%ld, value =%ld, write locked value switch to 0x01\n", blockid, value);
		    value = 0x01;
		    setKeyValue(myHashTable, blockid, value);
                    lock = 1;
                } else {
		    lock = 0;
		}
        }
        if(indicator == READ_LOCK) { //0x1 mean read lock
		if(value == 0x01 || value == -1) {
			lock = 0; 	
                } else {
			LOG("blockid=%ld, value = %ld, read locked value switch to %d\n",blockid, value, ((value >> 1) +1) << 1);
                        value = ((value >> 1) +1) << 1;
                        setKeyValue(myHashTable, blockid, value);
                        lock = 1;
		}
        } 
	if(indicator == RELEASE_LOCK) { //0x0 mean release lock
		if(value == 0x01) { 
			LOG("blockid=%ld, release write lock, switch to 0\n", blockid);
			value = 0;
			setKeyValue(myHashTable, blockid, value);
		} else if (value != 0x0 && value !=-1) {
			LOG("blockid=%ld, release read lock, value swtiched to %d\n",  blockid, ((value >>1) - 1)<<1);
                        value = ((value >>1) - 1)<<1;
                        setKeyValue(myHashTable, blockid, value);
		}
        }
	// release lock w/o response
	if (indicator != 0x0) {	
       		 write(socket, &lock, sizeof(lock));
	}
}

int main(int argc, char *argv[])
{
    struct sockaddr_in local;
    struct sockaddr_in peer;
    int sock_fd = 0, new_fd = 0;
    int ret = 0;
    socklen_t addrlen = 0;
    int x;
    
    HashTable* myHashTable = createHashTable();
    if (!myHashTable) {
        printf("Failed to create hash table.\n");
        exit(EXIT_FAILURE);
    }
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket error");
        return -1;
    }
    
    memset(&local, 0, sizeof(struct sockaddr_in));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(8888);
    
    ret = bind(sock_fd, (struct sockaddr *)&local, sizeof(struct sockaddr));
    if (ret == -1) {
        close(sock_fd);
        perror("bind error");
        return -1;
    }
    
    ret = listen(sock_fd, LISTEN_BACKLOG);
    if (ret == -1) {
        close(sock_fd);
        perror("listen error");
        return -1;
    }
    
    fd_set rfds;
    fd_set rfds_storage;
    FD_ZERO(&rfds_storage);
    FD_SET(sock_fd, &rfds_storage);
    
    int max_fd = sock_fd;
    while (1) {
        rfds = rfds_storage;
        struct  timeval tv = {.tv_sec = 5, .tv_usec = 0};
        ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select error");
            break;
        } else if (ret == 0) {
            continue;
        } 
        
        for (int fd = 0; fd < max_fd + 1; fd++) {
            if (FD_ISSET(fd, &rfds)) {
                if (fd == sock_fd) {
                    addrlen = sizeof(peer);
                    new_fd = accept(sock_fd, (struct sockaddr *)&peer, &addrlen);
                    if (new_fd == -1) {
                        perror("accept error");
                        continue;
                    }
                    FD_SET(new_fd, &rfds_storage);
                    max_fd = (new_fd <= max_fd) ? max_fd : new_fd;
                } else {
		    ret = recv(fd, &x,sizeof(int), 0);
                    if (ret <= 0) {
                        close(fd);
                        FD_CLR(fd, &rfds_storage);
                    } else {
			handle_lock(myHashTable, x, fd);
                    }
                }
            }
        }
    }
    
    for (int fd = 0; fd < max_fd + 1; fd++) {
        if (FD_ISSET(fd, &rfds)) {
            printf("clsoe fd:%d\n", fd);
            close(fd);
        }
    }
    
    FD_ZERO(&rfds);
    FD_ZERO(&rfds_storage);
    
    return 0;
}
