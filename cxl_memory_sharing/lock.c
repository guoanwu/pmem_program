#include "lock.h"

int socket_connect(int port, char * ip_addr)
{
    int sock = 0;
    struct sockaddr_in server_addr;
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    return sock;
}

void writelock(int sock, int uuid)
{
	int lock = 0;
	while (1) {
            //get the write lock
            lock = uuid <<2 | WRITE_LOCK;
            if (send(sock, &lock, sizeof(int), 0) < 0) {
                printf("send fail\n");
             }
             // 接收响应
             read(sock, &lock, sizeof(int));
             if (lock == 1) {
                 break;
             }
        }
	return;
}

void readlock(int sock, int uuid)
{
	int lock = 0;
	//get the read lock, until the read lock get
        while (1) {
            lock = uuid << 2 | READ_LOCK;
            if (send(sock, &lock, sizeof(int), 0) < 0) {
            	printf("send fail\n"); 
	    }

            read(sock, &lock, sizeof(int));
            if (lock == 1) {
                break;
            }
        }
	return;
}

void releaselock(int sock, int uuid)
{
	int lock = 0;
	// release the write lock, if the lock received.
        lock = uuid << 2 | RELEASE_LOCK;
        // release lock
        if (send(sock, &lock, sizeof(int), 0) < 0) {
            printf("send fail\n");
        }
	return;
}

void senderror(int sock, int uuid) 
{
        int lock = 0;
        // release the write lock, if the lock received.
        lock = uuid << 2 | ERROR_LOCK;
        // release lock
        if (send(sock, &lock, sizeof(int), 0) < 0) {
            printf("send fail\n");
        }
        return;

}	
