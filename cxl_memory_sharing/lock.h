#ifndef __LOCK_H__
#define __LOCK_H__
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef enum {
  RELEASE_LOCK = 0,
  READ_LOCK,
  WRITE_LOCK,
  ERROR_LOCK
} lock_type;


int socket_connect(int port, char * ip_addr);
void writelock(int sock, int uuid);
void readlock(int sock, int uuid);
void releaselock(int sock, int uuid);
void senderror(int sock, int uuid);
#endif
