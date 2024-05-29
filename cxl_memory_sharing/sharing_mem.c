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

int main() {
    unsigned int total_size = BLOCK_SIZE;
    unsigned int mmap_size = BLOCK_SIZE * BLOCK_NUM + 64;
    
    int fd = open("/mnt/famfs/lock_file", O_RDWR, S_IRWXU);
    int prot = PROT_READ|PROT_WRITE;
    char *addr =(char *) mmap(NULL, mmap_size, prot, MAP_SHARED|MAP_POPULATE, fd, 0);
    printf("the /mnt/famfs/lock_file address=%p\n", addr);
    addr =(char *) ((long int)addr & ~(FLUSH_ALIGN-1)); //make the address 64 bytes aligned.
    char * start =addr;

    int fd1 = open("/mnt/famfs1/lock_file", O_RDWR, S_IRWXU);
    char *addr1 =(char *) mmap(NULL, mmap_size, prot, MAP_SHARED|MAP_POPULATE, fd1, 0);
    printf("the /mnt/famfs1/lock_file address=%p\n", addr1);
    addr1 =(char *) ((long int)addr & ~(FLUSH_ALIGN-1)); //make the address 64 bytes aligned.
    
    char * start1 =  addr1;
    printf("mmap size=0x%x\n", mmap_size);

    int i;
    char * data = (char *) malloc(BLOCK_SIZE);

    while (1) { 
        
	i = random_select_block();

	addr = start + i * total_size;	
	fill_block_data(addr, BLOCK_SIZE);
        
	addr1 = start1 + i * BLOCK_SIZE;
	cflushopt(addr1 , BLOCK_SIZE);
        memcpy(data, addr1, BLOCK_SIZE); //read the data from the shared file to DRAM
	uint32_t crc32 = calculate_crc32(data, BLOCK_SIZE - sizeof(uint32_t));
        uint32_t data_crc32 = *(uint32_t *) (data + BLOCK_SIZE -sizeof(uint32_t));
        uint32_t source_crc32 = *(uint32_t *)(addr1 + BLOCK_SIZE - sizeof(uint32_t));
	printf("block=%d, calculated crc32=0x%x, source_crc32=0x%x\n", i, crc32, source_crc32);
	if(crc32 != source_crc32) printf("some error happen\n\n");    	

    	//assert(crc32 == source_crc32);
    }
     
    return 0;
}
