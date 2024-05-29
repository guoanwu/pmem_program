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
#include <time.h>
#include "rw_info.h"
#include "crc.h"
#include "log-timestamp.h"

int random_select_block() {
    time_t current_time = time(NULL);
    srandom((unsigned int) current_time);
    int randomNumber = (int) random();
    return randomNumber % BLOCK_NUM;
}

void fill_buffer_random(void * addr ,size_t size) {
    time_t current_time = time(NULL);
    srandom((unsigned int) current_time);
    char* allocatedMemory = (char *)addr;

    for(int bufferIndex = 0; bufferIndex < size; bufferIndex++) {
        char randomNumber = (char) random();
        allocatedMemory[bufferIndex] = randomNumber;
    }

    //char randomNumber = (char) random();
    //memset(allocatedMemory,randomNumber, size);
}


uint32_t fill_block_data(void *block,size_t block_size) {
    uint8_t * data =(uint8_t *) malloc(block_size); 
    fill_buffer_random(data,block_size-sizeof(uint32_t));
    uint32_t crc32 = calculate_crc32(data,block_size-sizeof(uint32_t));
    *(uint32_t *)(data + block_size-sizeof(uint32_t)) = crc32;
    memcpy(block, data, block_size);
    cflush(block, block_size);
    free(data);
    return crc32;
}

#ifndef HW_CACHE_COH

#define FLUSH_OPT
static inline void 
__cflush(void * addr, size_t len)
{
   long int uptr;
   for(uptr =(long int)addr & ~(FLUSH_ALIGN-1) ; uptr <  (long int)addr+len-64; uptr+=FLUSH_ALIGN)
   {
     _mm_clflushopt((void *)uptr);
   }
   _mm_sfence();
   _mm_clflushopt((void *)uptr);
   char a = *(char *)uptr;
}

static inline void
__cflushopt(void * addr, size_t len)
{
   long int uptr;
   for(uptr =(long int)addr & ~(FLUSH_ALIGN-1) ; uptr <  (long int)addr+len; uptr+=FLUSH_ALIGN)
   {
     _mm_clflushopt((void *)uptr);
   }
}

static inline void
__flush_processor_cache(const void *addr, size_t len)
{
    int64_t i;
    const char *buffer = (const char *)addr;

    /* Flush the processor cache for the target range */
    for (i=0; i<len; i+=CL_SIZE)
    	//_mm_clflushopt((void *)&buffer[i]);
	__builtin_ia32_clflush(&buffer[i]);

}

void cflush(void * addr, size_t len)
{
#ifndef FLUSH_OPT    
    __sync_synchronize();
    __flush_processor_cache(addr, len);
    __sync_synchronize();
#else
    __cflush(addr, len);   
#endif
}


void cflushopt(void * addr, size_t len)
{
#ifndef FLUSH_OPT	
    __flush_processor_cache(addr, len);
    __sync_synchronize();
#else
    __cflushopt(addr, len);    
#endif
}
#else
void cflush(void * addr, size_t len)
{
    return;
}
void cflushopt(void * addr, size_t len)
{
    return;
}
#endif

// 定义用于转换为16进制表示的辅助方法
static char *toHex(unsigned char byte)
{
    static char hex[17] = "0123456789ABCDEF";
    return &hex[(byte >> 4U) & 0xF];
}

void printMemory(void* ptr, size_t siz)
{
    unsigned char* currentPtr = (unsigned char*)ptr;

    for(size_t i=0; i<siz ; ++i )
    {
        printf("%s ", toHex(*currentPtr));

        // 每隔一定数量输出换行以便于阅读
        if((i+1)%16 == 0 || i==siz-1){
          putchar('\n');

        // 对每组后的剩余空间填充空格便于整齐排列
        int remainingSpaces = ((i + 1) % 16)?(16 - (i + 1) % 16):0;
        while(--remainingSpaces >= 0){
           putchar(' ');
        }

        // 输出对应位置可能的ASCII字符
        printf("| ");
        for(int j=(int)(i-(i%16));j<=i;++j){
          if(currentPtr[j]>= '!' && currentPtr[j]<= '~')
            putchar(currentPtr[j]);
          else
            putchar('.');
       }

       puts(" |");
    }

    ++currentPtr;
  }
}

