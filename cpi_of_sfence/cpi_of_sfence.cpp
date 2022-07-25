#include<time.h>
#include<stdlib.h>
#include<stdio.h>
#include<chrono>
#include<iostream>
#include<pthread.h>
#include<immintrin.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<pthread.h>
#include<libpmem.h>
#include<immintrin.h>
#include<string.h>

#define WRITE_BUFFER 1024*1024*1024 

static void write_buffer(void * dest, void * buf, size_t size,int sfence) 
{
   pmem_memcpy(dest,buf,size,PMEM_F_MEM_NONTEMPORAL|PMEM_F_MEM_NODRAIN);
   if(sfence)
   	_mm_sfence();
}

int main(int argc, char * argv[])
{
    auto start=std::chrono::steady_clock::now();
    auto stop=std::chrono::steady_clock::now();
    std::chrono::duration<double> diff=stop-start;
    if(argc != 5) {
      printf("usage:cpi_of_sfence mountpointer blocksize repeat sfence\n");
      printf("mountpointer is the namespace (fsdax) mount pointer such as /mnt/pmem0\n");
      return 0;
    } 
    
    char * mountpointer = argv[1];
    int blocksize = atoi(argv[2]);
    int repeat = atoi(argv[3]);
    int sfence = atoi(argv[4]);
    int buf_num = WRITE_BUFFER/blocksize;
    printf("mountpointer=%s, blocksize=%d,buf_num=%d, repeat=%d\n", mountpointer, blocksize, buf_num, repeat);

    int i, j;
    char *dest;
    char filename[32];
    size_t mapped_len;
    int is_pmem;
    void * mem_buf = malloc(blocksize);
    memset(mem_buf, 0x1, blocksize);
    
    start=std::chrono::steady_clock::now(); 
    //assume to overwrite 128GB data
    sprintf(filename,"%s/tmpfile",mountpointer);
    dest=(char *)pmem_map_file(filename,1024*1024*1024, PMEM_FILE_CREATE,
			0666, &mapped_len, &is_pmem);
    void *base = dest;
    if(dest!=NULL) {
      for(j = 0;j<repeat;j++) {
	base = dest;
        for (i=0;i<buf_num;i++) {
          write_buffer(base+i*blocksize, mem_buf, blocksize,sfence);
        }
      }
    } 
   
    stop=std::chrono::steady_clock::now();
    diff=stop-start;
    std::cout<<"write 1G buffer with " << repeat << " times take time: "<<diff.count()<<std::endl;
    
    return 0;
}
