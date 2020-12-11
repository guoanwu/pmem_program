#include<time.h>
#include<stdlib.h>
#include<stdio.h>
#include <chrono>
#include<iostream>
#include<pthread.h>
#include<immintrin.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<pthread.h>
#include<libpmem.h>

typedef struct rand_gen {
  char * addr;
  size_t len;
}rand_gen_t;

static void * generate_rand(void *arg) 
{
    int i,j;
    rand_gen_t * rand_s = (rand_gen_t *) arg;
    int * buf=(int *)rand_s->addr;
    size_t len=rand_s->len/sizeof(int);
    //if the size is not 4 times, some tail data need to overwrite
    //size_t tail=rand->len%sizeof(int);
    srand((int)time(0));
    for(i=0;i<len;i++)
    {
        j=rand();
        buf[i]=j;
    }
    //printf("buf=%p,size=%d\n",buf,len);
    return NULL;
}


static void thread_gen_rand(int thread,char * buf, size_t buf_size) 
{
    int i,j;
    
    //how much thread to generate the rand
    //4 thread performance is less than 1 thread, why?? 
    int threads= 1;
    pthread_t *id =(pthread_t *) malloc(threads * sizeof(pthread_t));
    rand_gen_t * rand_s = (rand_gen_t *)malloc(threads * sizeof(rand_gen_t));
    //how much dram buffer size for the generated data
    int ret;
    int gen_len=buf_size/threads; //must divided.
    
    for(i=0;i<threads;i++) {
        rand_s[i].addr=buf+i*gen_len;
	rand_s[i].len = gen_len;
	ret=pthread_create(&id[i],NULL,generate_rand,(void *)&rand_s[i]);
    	if(ret!=0) return ;
    }

    for(i=0;i<threads;i++) {
	pthread_join(id[i],NULL);
    }

    free(id);
    free(rand_s);
}

typedef struct buf_write {
    char * base;
    size_t size_of_4k;
    char * buf_4k;
}buf_write_t;


static void * thread_write_buffer(void * args) 
{
   buf_write_t * bufwrite=(buf_write_t *)args;
   int size_of_4k = bufwrite->size_of_4k;
   char * base = bufwrite->base;
   char * buf= bufwrite->buf_4k;
   for(int k=0;k<size_of_4k;k++) {
     pmem_memcpy(base,buf,4096,PMEM_F_MEM_NONTEMPORAL|PMEM_F_MEM_NODRAIN);
     base+=4096;
   } 
   return NULL;  
}


static void write_4k_num(char * base,int size_of_4k) {
    int i;
    char * buf=(char *)malloc(4096);
    thread_gen_rand(1,buf,4096);       
    // use 2 threads to write the 4k number;
    int ret;
    int threads=2;
    pthread_t *id = (pthread_t *) malloc(threads * sizeof(pthread_t));
    buf_write_t * buf_s = (buf_write_t *)malloc(threads * sizeof(buf_write_t));
    
    for(i=0;i<threads;i++) {  
      buf_s[i].base = base+i*(size_of_4k/threads);
      buf_s[i].size_of_4k = size_of_4k/threads;
      buf_s[i].buf_4k = buf;
      if(i==threads-1) {
        buf_s[i].size_of_4k=size_of_4k-i*size_of_4k/threads;
      }
      ret=pthread_create(&id[i],NULL,thread_write_buffer,(void *)&buf_s[i]);
      if(ret!=0) return ;
    }
    for(i=0;i<threads;i++) {
        pthread_join(id[i],NULL);
    }
    
    free(id);
    free(buf_s);
    free(buf);
    return;
}

int main(int argc, char * argv[])
{
    int i,j;
    
    auto start=std::chrono::steady_clock::now();
    auto stop=std::chrono::steady_clock::now();
    std::chrono::duration<double> diff=stop-start;
    if(argc ==1 || argc >2) {
      printf("usage:random_namespace mountpointer\n");
      printf("mountpointer is the namespace (fsdax) mount pointer such as /mnt/pmem0\n");
      return 0;
    } 
    char * mountpointer = argv[1];

    char *dest;
    char filename[32];
    size_t mapped_len;
    int is_pmem;
    
    start=std::chrono::steady_clock::now(); 
    for(j=0;j<3;j++) {
      i=0;
      while(1) {
    	//assume to overwrite 128GB data
	sprintf(filename,"%s/tmpfile%d",mountpointer,i);
    	dest=(char *)pmem_map_file(filename,1024*1024*1024, PMEM_FILE_CREATE,
			0666, &mapped_len, &is_pmem);
        if(dest!=NULL) {
	    write_4k_num(dest,mapped_len/4096);
	} else {
	  break;
	}
	i++;
      }
      printf("mapped 1G num=%d, mapped_len=%d\n",i,mapped_len);    
    }
   
    stop=std::chrono::steady_clock::now();
    diff=stop-start;
    std::cout<<"128G pmem written rand 3 times take time "<<diff.count()<<std::endl;
    
    return 0;
}
