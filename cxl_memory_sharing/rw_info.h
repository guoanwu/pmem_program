#ifndef __RW_INFO_H__
#define __RW_INFO_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BLOCK_NUM 255 
#define BLOCK_SIZE 8*1024*1024
#define FLUSH_ALIGN 64
#define META_SIZE 64

//if use the hardware cache coherency, define the flag
#define HW_CACHE_COH

uint32_t fill_block_data(void *block,size_t block_size);
void cflush(void * addr, size_t len);
void cflushopt(void * addr, size_t len);
void printMemory(void* ptr, size_t siz);
void fill_buffer_random(void * addr ,size_t size);
int random_select_block();

#endif
