#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

uint64_t ntime_since(const struct timespec *s, const struct timespec *e)
{
       int64_t sec, nsec;

       sec = e->tv_sec - s->tv_sec;
       nsec = e->tv_nsec - s->tv_nsec;
       if (sec > 0 && nsec < 0) {
               sec--;
               nsec += 1000000000LL;
       }
       /*
        * time warp bug on some kernels?
        */
       if (sec < 0 || (sec == 0 && nsec < 0))
               return 0;

       return nsec + (sec * 1000000000LL);
}

#define LOG_NUM 10000

unsigned char * logs[LOG_NUM];
const unsigned char allChar[63]="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
 
void generateString(unsigned char* dest,const unsigned int len)
{
    unsigned int cnt,randNo;
    srand((unsigned int)time(NULL));
    for(cnt=0;cnt<len;cnt++)
    {
      randNo=rand()%62;
      *dest=allChar[randNo];
      dest++;
    }
    *dest='\0';
}  

//generate LOG_NUM logs with different size from 50~250
void gen_log()
{
    int i,size;
    unsigned char * dest;
    srand((unsigned int)time(NULL));
    //generate logs     
    for(i=0;i<LOG_NUM;i++) 
    {
      size=rand()%(250-50+1)+50; //50~250 bytes.
      dest=(unsigned char *)malloc(size+1);
      generateString(dest, size);
      logs[i]=dest;
    }
}

int main()
{
    int fd, size;
    char *buffer;
    struct timespec start, end;
    int i=100;
    int j;
    
    gen_log();
    //write log, we must append write
    fd = open("/mnt/nvme0/wal.log", O_WRONLY|O_CREAT|O_APPEND);
    
    clock_gettime(CLOCK_REALTIME, &start);
    //1M log write into the log
    while(i--) {
      for(j=0;j<LOG_NUM;j++) 
      {
         write(fd,logs[j],strlen(logs[j]));
	 fdatasync(fd); //in the normal code, we need to fdatasync and with the wal accellerate, we need to use the AOFGUARD_DISABLE_SYNC=yes to ignore this sync; the data sync is called in the write
      }
    }
    clock_gettime(CLOCK_REALTIME, &end);
    printf("the durating for 1m logs=%ld\n",ntime_since(&start,&end));
  
    //fd: the file is closed 
    close(fd);
    
    //read progress:since when open the log again, it will copy the data in the pmem to the NVMe, so the O_RDWR must be used. 
    fd = open("/mnt/nvme0/wal.log", O_RDWR);
    
    struct stat stat;
    
    fstat(fd,&stat);
    printf("fd=%d,filesize=%ld read the log from the head with the size 300 \n",fd,stat.st_size);
    
    //lseek(fd, 0, SEEK_SET);
    buffer=malloc(300); 
    size = read(fd, buffer, 300);  //read data to a buffer
    printf("size=%d, buffer=%s, bufferlen=%ld\n",size, buffer, strlen(buffer));
    close(fd);
    free(buffer);
}
