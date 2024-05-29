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

int main(int argc, char **argv)
{
    long int microseconds;
    struct timeval startVal, endVal;
    unsigned int size = 0u;
    unsigned int meta_size = 64; 
    unsigned int total_size = BLOCK_SIZE; 
    unsigned int mmap_size = meta_size + total_size + 64;

    if (argc!= 2 ) {
        printf("please include the parameter: filepath \n");
        printf("example: ./meta_write /mnt/famfs/test_file");
        return 0;
    }
    char * filepath =(char *)argv[1];

    int fd = open(filepath, O_RDWR, S_IRWXU);
 
    int prot = PROT_READ|PROT_WRITE;
    char *addr =(char *) mmap(NULL, mmap_size, prot, MAP_SHARED|MAP_POPULATE, fd, 0);
    printf("the test file address=%p\n", addr);
    memset(addr,0x0,meta_size);
    cflush(addr,64);

    char * data_addr = (char *)addr + 64;
    char * dram_source=(char *) malloc(total_size);

    // prepare 2M dram with the data
    while (1) {
         fill_buffer_random((void *)dram_source,total_size-sizeof(uint32_t));
	 uint32_t crc32 = calculate_crc32(dram_source,total_size-sizeof(uint32_t));
         *(uint32_t *)(dram_source + total_size-sizeof(uint32_t)) = crc32;
	 cflushopt(addr, 64);
	 char write_flag = *(char *)addr;
	 if (write_flag == 0x00) {
	 	printMemory(dram_source+total_size-256, 256);
		gettimeofday(&startVal, NULL);
		memcpy(data_addr, dram_source, total_size); // write the share file	 	
		cflush(data_addr, total_size); // flush the data
		
		memset(addr,0x10,64);
		cflush(addr, 64);
		gettimeofday(&endVal, NULL);
    		microseconds = (endVal.tv_sec - startVal.tv_sec) * 1000000 + (endVal.tv_usec - startVal.tv_usec);
		printf("write total_size: %d, crc32 value: %d done! time=%ld microseconds\n", total_size-4, crc32, microseconds);
	 }
	 else {
	 	usleep(1);
	 }
    }
 
    munmap(addr, mmap_size);
    free(dram_source);
 
    return 0;
}
