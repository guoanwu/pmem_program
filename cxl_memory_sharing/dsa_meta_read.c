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
#include "dml/dml.h"
#include "rw_info.h"
#include "dsa_move.h"

/*
* This example demonstrates application write to a share file 
*/

int main(int argc, char **argv)
{
    long int microseconds;
    struct timeval startVal, endVal;
 
    unsigned int size = 0u;
    unsigned int meta_size = META_SIZE; 
    unsigned int total_size = BLOCK_SIZE; 
    unsigned int mmap_size = meta_size + total_size + 64;

    if (argc!= 2 ) {
        printf("please include the parameter: filepath \n");
        printf("example: ./dsa_meta_read /mnt/famfs/test_file");
        return 0;
    }
    char * filepath =(char *)argv[1];
    int fd = open(filepath, O_RDWR, S_IRWXU);
    int prot = PROT_READ|PROT_WRITE;
    char *addr =(char *) mmap(NULL, mmap_size, prot, MAP_SHARED|MAP_POPULATE, fd, 0);
    printf("the test file address=%p\n", addr);

    char * data_addr = (char *)addr + 64;
    char * dram_source=(char *) malloc(total_size);
    dml_job_t ** jobs = init_dsa(DSA_JOB_NUM);

    // prepare 2M dram with the data
    while (1) {
	 cflushopt(addr, 64);
	 char read_flag = *(char *)addr;
	 if (read_flag == 0x10) {
    		gettimeofday(&startVal, NULL);
		// clear the data address cache, read the data from the CXL memory instead of cache
		cflush(data_addr, total_size);		
		dsa_move_data(jobs, DSA_JOB_NUM, data_addr, dram_source, total_size, DSA_MOVE_BLOCK);

		memset(addr,0x00,64);
    		cflush(addr,64);
		gettimeofday(&endVal, NULL);

    		microseconds = (endVal.tv_sec - startVal.tv_sec) * 1000000 + (endVal.tv_usec - startVal.tv_usec);
		uint32_t crc32 = calculate_crc32(dram_source, total_size-sizeof(uint32_t));
		printf("\nread the data from the share file test_file_dsa to dram\n");	
		printMemory(dram_source+total_size-256, 256);
		uint32_t source_crc32 = *(uint32_t *)(dram_source + total_size - sizeof(uint32_t));
		assert(crc32 == source_crc32);

		printf("read total_size: %d, source_crc32: %d, crc32: %d, time: %ld microseconds\n", total_size, source_crc32, crc32, microseconds);
		
	 }
	 else {
	 	usleep(1);
	 }
    }
 
 
    munmap(addr, mmap_size);
    free(dram_source);
 
    return 0;
}
