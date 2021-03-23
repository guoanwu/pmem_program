#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libpmem.h>
#include <memkind.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define BLOCK_PAGE_NUM 1024
#define REQ_BLOCK 10
#define REQ_PAGE 0x3ff
#define DISK_BLOCK_NUM 1048576 //1024*1024 blocks 
#define PMEM_SIZE 100*1024*1024*1024UL


/*****************************************
page are in the PMEM (allocated from the memkind) 
 */
typedef struct block_page {
  unsigned char * page;
}block_page_t;
typedef struct block_data {
  block_page_t pages[BLOCK_PAGE_NUM];
  uint32_t cached_pages_cnt;  //if the count over one threshold, might need to write back to the SSD.   
}block_data_t;

typedef struct disk_s {
  block_data_t blocks[DISK_BLOCK_NUM];
}disk_t;

disk_t * disk;
struct memkind *pmem_kind;

//初始化，传入的是文件的名称，文件名称也可以定义在头文件中间。
int cbs_init(const char * filename)
{
  int i;
  int err = memkind_create_pmem(filename, PMEM_SIZE, &pmem_kind);
  if (err) {
	perror("memkind_create_pmem()");
	return 1;
  }
  disk=(disk_t *) memkind_calloc(MEMKIND_DEFAULT,1,sizeof(disk_t)); 
  if(disk == NULL) {
    return 1;
  }
  return 0;
}

int get_cached_count() 
{
  int i;
  int cnt=0;
  for(i=0;i<DISK_BLOCK_NUM;i++) {
    cnt+=disk->blocks[i].cached_pages_cnt;
  }
  return cnt;
}

//写入或者更新一个页，输入req_id表示更新到什么地方，content表示更新的内容
int write_req(uint64_t req_id, unsigned char * content) {
  //req_id>>10是block_id(1024 pages/block); req_id&0x3ff是该block中的page_id.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  
  if(block_id > DISK_BLOCK_NUM) {
    printf(" write req_id is not valid and over the disk range\n");
    return -1;
  }
  unsigned char * page=disk->blocks[block_id].pages[req_page_id].page;
  if(page == NULL) { 
	page = (unsigned char *)memkind_malloc(pmem_kind, PAGE_SIZE);
	disk->blocks[block_id].pages[req_page_id].page=page;
  }
  if (page != NULL) {
	pmem_memcpy_nodrain(page,content,PAGE_SIZE);
	//memcpy(page,content,PAGE_SIZE);
  } 
  return 0;
}

void * read_req(uint64_t req_id) {
  // req_id and content; only after the init success, then this API can be called.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  if(block_id > DISK_BLOCK_NUM) {
    printf("read req_id is not valid and over the disk range\n");
    return NULL;
  }

  return disk->blocks[block_id].pages[req_page_id].page;
}

void delete_page(uint64_t req_id) {
  // req_id and content; only after the init success, then this API can be called.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  
  if(block_id > DISK_BLOCK_NUM) {
    printf(" write req_id is not valid and over the disk range\n");
    return;
  }
  
  unsigned char * page=disk->blocks[block_id].pages[req_page_id].page;
  
  if (page != NULL) {
	memkind_free(pmem_kind,page);
	disk->blocks[block_id].pages[req_page_id].page=NULL;
  } 
}

#define WRITE_COUNT 100000
#define OVERWRITE_COUNT 10000
int main() 
{
  // calculate the time
  unsigned char * page_content=(unsigned char *)malloc(4096);
  uint64_t i=0;
  auto start=std::chrono::steady_clock::now();
  auto stop=std::chrono::steady_clock::now();
  std::chrono::duration<double> diff=stop-start;

  char * read_content;
  memset(page_content,0xab,4096);
  
  start=std::chrono::steady_clock::now();
  cbs_init("/mnt/pmem0");
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"cbs_init time "<<diff.count()<<std::endl; 
  std::cout<<"cached page count" << get_cached_count()<<std::endl;

  start = std::chrono::steady_clock::now();
  for(i=0;i<WRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"write_req time "<<diff.count()/WRITE_COUNT<<std::endl;

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
    read_content=(char *)read_req(i);
    memcpy(page_content,read_content,PAGE_SIZE); 
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite read_req take time "<<diff.count()/OVERWRITE_COUNT<<std::endl;
  printf("the page should fill with paten 0xcd, 0x%x\n", page_content[0]);

  start = std::chrono::steady_clock::now();
  for(i=OVERWRITE_COUNT;i<WRITE_COUNT;i++) {
    read_content=(char *)read_req(i);
    memcpy(page_content,read_content,PAGE_SIZE);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite->write count read_req take time "<<diff.count()/(WRITE_COUNT-OVERWRITE_COUNT)<<std::endl;
  printf("the page should fill with patern 0xab, 0x%x\n", page_content[0]);

  for(i=0;i<WRITE_COUNT;i++) {
    delete_page(i);
  }
  ////sleep(10);
  start = std::chrono::steady_clock::now();
  for(i=0;i<WRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"write_req time "<<diff.count()/WRITE_COUNT<<std::endl;

  memset(page_content,0xcd,4096);

  start = std::chrono::steady_clock::now();
  for(i=0;i<OVERWRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite write_req update take time "<< diff.count()/OVERWRITE_COUNT<<std::endl;

  //start = std::chrono::steady_clock::now();
  //for(i=0;i<WRITE_COUNT;i++) {
  //  delete_page(i);
  //}
  //stop=std::chrono::steady_clock::now();
  //diff=stop-start;
  //std::cout<<"delete write count take time "<<diff.count()/WRITE_COUNT<<std::endl;
  return 0;
}

