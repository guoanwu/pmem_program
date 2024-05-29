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
#include "rw_info.h"
#include "log-timestamp.h"
#include "lock.h"

int main(int argc, char **argv)
{
    long int microseconds;
    struct timeval startVal, endVal;

    if (argc!= 2) {
    	printf("please include the parameter: filepath\n");
	printf("example: ./share_file_write /mnt/famfs/lock_file");
	return 0;
    }
    char * filepath =(char *)argv[1];
    
    int sock = socket_connect(8888, "127.0.0.1");
    if (sock == -1) {
        return 1;
    }

    int i;
    int lock;
    unsigned int size = 0u;
    unsigned int mmap_size = BLOCK_SIZE * BLOCK_NUM + 64;

    int fd = open(filepath, O_RDWR, S_IRWXU);
    int prot = PROT_READ|PROT_WRITE;
    char *addr =(char *) mmap(NULL, mmap_size, prot, MAP_SHARED|MAP_POPULATE, fd, 0);
    printf("the test file address=%p\n", addr);

    char * data = (char *) malloc(BLOCK_SIZE);
    char * start = addr; 
    
    while (1) {
	gettimeofday(&startVal, NULL);
	i = random_select_block();
	readlock(sock,i);	
	LOG("block %d get the read lock\n", i);
	addr = start + i * BLOCK_SIZE;
        cflushopt(addr, BLOCK_SIZE);		
	memcpy(data, addr, BLOCK_SIZE); //read the data from the shared file to DRAM	 	
	    
	gettimeofday(&endVal, NULL);
        microseconds = (endVal.tv_sec - startVal.tv_sec) * 1000000 + (endVal.tv_usec - startVal.tv_usec);
	uint32_t crc32 = calculate_crc32(data, BLOCK_SIZE - sizeof(uint32_t));
	uint32_t data_crc32 = *(uint32_t *) (data + BLOCK_SIZE - sizeof(uint32_t));
	uint32_t source_crc32 = *(uint32_t *)(addr + BLOCK_SIZE - sizeof(uint32_t));
	LOG("read block=%d, size: %d, time=%ld us, calculated crc32=0x%x, data_crc32=0x%x, source crc32=0x%x\n", i, BLOCK_SIZE , microseconds, crc32, data_crc32, source_crc32);
	if(crc32 != source_crc32) {
	       	LOG("crc32 check fail\n");
		printMemory(data+BLOCK_SIZE -128, 128);
		//assert(crc32 == source_crc32);
	}
	releaselock(sock,i);
	LOG("block %d release the read lock\n", i);
    }
 
 
    munmap(start, mmap_size);
    free(data);
 
    return 0;
}
