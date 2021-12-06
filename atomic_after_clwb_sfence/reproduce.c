#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <immintrin.h>
#include <libpmem.h>

#define FLUSH_ALIGN 64
#define NVM_FILE "/mnt/pmem0/src_file"
int page_size = 4 * 1024;
//int page_size = 512;
long long page_count = 1024;
//#define DATA_SIZE 128
#define DATA_SIZE 4096
char data_buffer[DATA_SIZE];

#define _mm_clflushopt(addr)              \
    asm volatile(".byte 0x66; clflush %0" \
                 : "+m"(*(volatile char *)addr));
#define _mm_clwb(addr)                     \
    asm volatile(".byte 0x66; xsaveopt %0" \
                 : "+m"(*(volatile char *)addr));

/*
 *  * flush_clwb -- (internal) flush the CPU cache, using clwbs
 *   */
static void
flush_clwb(const void *addr, size_t len)
{

    uintptr_t uptr;

    for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
         uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN)
    {
        _mm_clwb((char *)uptr);
    }
}

int test_atomic = 0;
void atmoic_add(int *ptr)
{
    __sync_add_and_fetch(&test_atomic, 1);
}

int main()
{
    unlink(NVM_FILE);
    int fd = open(NVM_FILE, O_CREAT | O_RDWR, 0644);
    size_t file_size = page_size * page_count;
    fallocate(fd, 0, 0, file_size);
    char *buffer = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //memset(buffer, 0, file_size);
    pmem_memset_persist(buffer,0,file_size);

    long long div_sum = 0;
    for (int loop = 0; loop < 300; loop++)
    {
        for (int offset = 0; offset <= page_size - DATA_SIZE; offset += DATA_SIZE)
        {

            for (int i = 0; i < page_count; i++)
            {
                void *ptr = buffer + page_size * i + offset;
                //pmem_memcpy_persist(ptr,data_buffer,DATA_SIZE);
		memcpy(ptr, data_buffer, DATA_SIZE);
                flush_clwb(ptr, DATA_SIZE);
                _mm_sfence();
                //multi_thread atomic?         
#ifdef LOCK_BEFORE_PAUSE
                atmoic_add(&test_atomic);
#endif

#ifndef LOCK_WITHOUT_PAUSE
                // doing other work for a long time
                for (int i = 0; i < 10; i++)
                {
                    _mm_pause();
                }
#endif

#ifdef LOCK_AFTER_PAUSE
                atmoic_add(&test_atomic);
#endif
            }
        }
    }
    return 0;
}
