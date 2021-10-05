#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ALLOC_NUM 16384
#define LOOP 10000
#define MAX_BLOCK 16 //128k
#define THREAD_NUM 32

void * memory_alloc(void *arg)
{
  int i,j,len,seek;
  uint64_t fail=0,sum=0;
  void * ptrs[ALLOC_NUM];
  seek = time(NULL);
  for(j=0;j<LOOP;j++) {
    for(i=0;i<ALLOC_NUM;i++) {
      len = 1<<(rand_r(&seek)%MAX_BLOCK);
      ptrs[i]=malloc(len);
      if(ptrs[i]) {
        sum +=len;
      }else {
        fail ++;
      }
    }
    for(i=0;i<ALLOC_NUM;i++) {
      free(ptrs[i]);
    }
  }
  printf("used bytes:%ldMB, fail=%ld\n",sum/1024/1024, fail);
  
  return NULL;
}

int main(int argc, char * argv[])
{
  pthread_t threads[THREAD_NUM];
  int tn = 1,i=0;
  int ret=0;
  if(argc==1 || argc>2) {
    printf("usage: memory_mode_test thread_num\n");
    return 0;
  }
  tn = atoi(argv[1]);
  if(tn > THREAD_NUM) {
      printf("Thread number set is over the THREAD_NUM(32) limit, use the max thread (32)\n");
      tn=THREAD_NUM;
  }

  for(i=0;i<tn;i++) {
      ret = pthread_create(&threads[i],NULL, memory_alloc, NULL);
      if(ret !=0) {
        printf("%d thread create failed\n",i);
	return 0;
      }
  }

  for(i=0;i<tn;i++) {
    pthread_join(threads[i],NULL);
  }
  return 0;
}


