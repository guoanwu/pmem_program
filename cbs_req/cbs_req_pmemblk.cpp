#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libpmemblk.h>
#include <iostream>
#include <chrono>
#include <cassert>


/* size of the pmemblk pool -- 1 GB */
#define POOL_SIZE ((uint64_t)((1<< 30)*100UL))

/* size of each element in the pmem pool */
#define ELEMENT_SIZE 4096

// pmem block pool
PMEMblkpool *pbp;
size_t nelements;

int cbs_init(const char * filename)
{
  /* create the pmemblk pool or open it if it already exists */
  pbp = pmemblk_create(filename, ELEMENT_SIZE, POOL_SIZE, 0666);

  if (pbp == NULL)
    pbp = pmemblk_open(filename, ELEMENT_SIZE);

  if (pbp == NULL) {
    perror(filename);
    return -1;
  }

  /* how many elements fit into the file? */
  nelements = pmemblk_nblock(pbp);
  printf("file holds %zu elements", nelements);  
  return 0;
}

int write_req(uint64_t req_id, unsigned char * content) {
  assert(req_id < nelements);
  if (pmemblk_write(pbp, content, req_id) < 0) {
    perror("pmemblk_write");
    return -1;
  }
  
  return 0;
}

void * read_req(uint64_t req_id, unsigned char * buf) {
  assert(req_id<nelements);
  /* read the block at index 10 (reads as zeros initially) */
  if (pmemblk_read(pbp, buf, req_id) < 0) {
    perror("pmemblk_read");
    return NULL;
  }
  return buf;
}

#define WRITE_COUNT 100000
#define OVERWRITE_COUNT 10000
int main() 
{
  // calculate the time
  unsigned char * read_content;
  unsigned char * page_content=(unsigned char *)malloc(4096);
  uint64_t i=0;
  auto start=std::chrono::steady_clock::now();
  auto stop=std::chrono::steady_clock::now();
  std::chrono::duration<double> diff=stop-start;

  memset(page_content,0xab,4096);
  
  start=std::chrono::steady_clock::now();
  cbs_init("/mnt/pmem0/cbs_pmblk");
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"cbs_init time"<<diff.count()<<std::endl; 
  
  //std::cout<<"cached page count"<<get_cached_count()<<std::endl;
  start = std::chrono::steady_clock::now();
  for(i=0;i<WRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"write_req time"<<diff.count()/WRITE_COUNT<<std::endl;

  memset(page_content,0xcd,4096);
  start = std::chrono::steady_clock::now();
  for(i=0;i<OVERWRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite write_req update take time "<< diff.count()/OVERWRITE_COUNT<<std::endl;

  start = std::chrono::steady_clock::now(); 
  for(i=0;i<OVERWRITE_COUNT;i++) {
    read_content=( unsigned char *)read_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite read_req take time "<<diff.count()/OVERWRITE_COUNT<<std::endl;
  printf("the page should fill with paten 0xcd, 0x%x\n", read_content[0]);

  start = std::chrono::steady_clock::now();
  for(i=OVERWRITE_COUNT;i<WRITE_COUNT;i++) {
    read_content=( unsigned char *)read_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite->write count read_req take time "<<diff.count()/(WRITE_COUNT-OVERWRITE_COUNT)<<std::endl;
  printf("the page should fill with patern 0xab, 0x%x\n", read_content[0]);

  pmemblk_close(pbp);
  return 0;
}
