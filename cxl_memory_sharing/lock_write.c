/*******************************************************************************
* Copyright (C) 2023 Intel Corporation
*
* SPDX-License-Identifier: MIT
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <immintrin.h>
#include "crc.h"
#include "log-timestamp.h"
#include "rw_info.h"
#include "lock.h"

int main(int argc, char **argv)
{
    long int microseconds;
    struct timeval startVal, endVal;
    int sock;

    if (argc!= 2 ) {
        printf("please include the parameter: filepath \n");
        printf("example: ./share_file_write /mnt/famfs/test_file");
        return 0;
    }
    char * filepath =(char *)argv[1];

    // Create socket
    if ((sock = socket_connect(8888, "127.0.0.1")) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
 

    int i;
    unsigned int size = 0u;
    unsigned int total_size = LOCK_BLOCK_SIZE; 
    unsigned int mmap_size = LOCK_BLOCK_SIZE * BLOCK_NUM + 64;
    
    int fd = open(filepath, O_RDWR, S_IRWXU);
    int prot = PROT_READ|PROT_WRITE;
    char *addr =(char *) mmap(NULL, mmap_size, prot, MAP_SHARED|MAP_POPULATE, fd, 0);
    printf("the filepath=%s address=%p\n", filepath, addr);
    char *start = addr;

    uint8_t * data = (uint8_t *) malloc(LOCK_BLOCK_SIZE);
    int lock;
    // prepare 2M dram with the data
    while (1) {
	i = random_select_block();	
	fill_buffer_random((void *)data, total_size-sizeof(uint32_t));	
    	uint32_t crc32 = calculate_crc32(data, LOCK_BLOCK_SIZE - sizeof(uint32_t));
    	*(uint32_t *)(data + LOCK_BLOCK_SIZE - sizeof(uint32_t)) = crc32;

	writelock(sock,i);

	gettimeofday(&startVal, NULL);
	addr = start + i * LOCK_BLOCK_SIZE;
	//uint32_t crc32 = fill_block_data(addr, LOCK_BLOCK_SIZE);
	memcpy(addr, data, LOCK_BLOCK_SIZE);
	cflush(addr, LOCK_BLOCK_SIZE);
	gettimeofday(&endVal, NULL);
    	microseconds = (endVal.tv_sec - startVal.tv_sec) * 1000000 + (endVal.tv_usec - startVal.tv_usec);
	printMemory(addr + LOCK_BLOCK_SIZE-128, 128);   
	LOG("write block %d, crc32=0x%x, total_size: %d, time=%ld microseconds\n", i, crc32, total_size, microseconds);

	releaselock(sock, i);
    }
 
    munmap(start, mmap_size);
    return 0;
}
