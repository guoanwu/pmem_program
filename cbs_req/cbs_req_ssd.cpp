#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libpmem.h>
#include <time.h>
#include <string.h>
#include <cassert>

#define PAGE_SIZE 4096
#define PMEM_SIZE 100*1024*1024*1024UL
//ssd 的mmap的基地址
char * pmem_base;
int is_pmem;

//初始化，传入的是文件的名称，文件名称也可以定义在头文件中间。
int cbs_init(const char * filename)
{
  size_t mapped_len, pagecache_num;
  //int is_pmem;
  int i;
  //将持久内存映射到虚拟地址空间并得到基地址pmem_base
  if ((pmem_base = (char *)pmem_map_file(filename, PMEM_SIZE,
                                            PMEM_FILE_CREATE, 0666,
                                            &mapped_len, &is_pmem)) == NULL) {
    perror("Pmem map file failed");
    return -1;
  }
  printf("pmem_map_file mapped_len=%ld, is_pmem=%ld\n", mapped_len, is_pmem);
  
  return 0;
}

//写入或者更新一个页，输入req_id表示更新到什么地方，content表示更新的内容
int write_req(uint64_t req_id, unsigned char * content) {
  //req_id>>10是block_id(1024 pages/block); req_id&0x3ff是该block中的page_id.
  uint64_t page_offset = req_id<<12; //req_id*4k is the offset of the page

  memcpy((char *)pmem_base+page_offset, content, PAGE_SIZE);  //从pagemeta中拿到4k page，将数据写入并持久化
  pmem_msync((char *)pmem_base+page_offset,PAGE_SIZE);
    
  return 0;
}

void * read_req(uint64_t req_id) {
  uint64_t page_offset=req_id<<12;
  return (void *)(pmem_base+page_offset);
}

#define WRITE_COUNT 100000
#define OVERWRITE_COUNT 10000
int main() 
{
  // calculate the time
  unsigned char * page_content=(unsigned char *)malloc(PAGE_SIZE);
  uint64_t i=0;
  auto start=std::chrono::steady_clock::now();
  auto stop=std::chrono::steady_clock::now();
  std::chrono::duration<double> diff=stop-start;

  start=std::chrono::steady_clock::now();
  cbs_init("/mnt/nvme0/cbs_ssd");
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"cbs_init time "<<diff.count()<<std::endl; 
  
  unsigned char * read_content;
  // read first
  start = std::chrono::steady_clock::now();
  for(i=0;i<OVERWRITE_COUNT;i++) {
    read_content=(unsigned char *)read_req(i);
    if(read_content) {
      memcpy(page_content,read_content,PAGE_SIZE);
      //assert(page_content[0]==0xcd);
    }
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite read_req take time "<<diff.count()/OVERWRITE_COUNT<<std::endl;
  printf("the page should fill with paten 0xcd, 0x%x\n", page_content[0]);

  start = std::chrono::steady_clock::now();
  for(i=OVERWRITE_COUNT;i<WRITE_COUNT;i++) {
    read_content=(unsigned char *)read_req(i);
    if(read_content) {
      memcpy(page_content,read_content,PAGE_SIZE);
      //assert(page_content[0]==0xab);
    }
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite->write count read_req take time "<<diff.count()/(WRITE_COUNT-OVERWRITE_COUNT)<<std::endl;
  printf("the page should fill with patern 0xab, 0x%x\n", page_content[0]);
  
  
  memset(page_content,0xab,PAGE_SIZE);
  

  start = std::chrono::steady_clock::now();
  for(i=0;i<WRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"write_req time "<<diff.count()/WRITE_COUNT<<std::endl;

  memset(page_content,0xcd,PAGE_SIZE);

  start = std::chrono::steady_clock::now();
  for(i=0;i<OVERWRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite write_req update take time "<< diff.count()/OVERWRITE_COUNT<<std::endl;

  start = std::chrono::steady_clock::now(); 
  for(i=0;i<OVERWRITE_COUNT;i++) {
    read_content=(unsigned char *)read_req(i);
    if(read_content) {
      memcpy(page_content,read_content,PAGE_SIZE);
      assert(page_content[0]==0xcd);
    }
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite read_req take time "<<diff.count()/OVERWRITE_COUNT<<std::endl;
  printf("the page should fill with paten 0xcd, 0x%x\n", page_content[0]);

  start = std::chrono::steady_clock::now();
  for(i=OVERWRITE_COUNT;i<WRITE_COUNT;i++) {
    read_content=(unsigned char *)read_req(i);
    if(read_content) {
      memcpy(page_content,read_content,PAGE_SIZE);
      assert(page_content[0]==0xab);
    }
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite->write count read_req take time "<<diff.count()/(WRITE_COUNT-OVERWRITE_COUNT)<<std::endl;
  printf("the page should fill with patern 0xab, 0x%x\n", page_content[0]);

  //start = std::chrono::steady_clock::now();
  //for(i=0;i<WRITE_COUNT;i++) {
  //  delete_page(i);
  //}
  //stop=std::chrono::steady_clock::now();
  //diff=stop-start;
  //std::cout<<"delete write count take time "<<diff.count()/WRITE_COUNT<<std::endl;
  return 0;
}

