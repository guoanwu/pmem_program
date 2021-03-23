#include <iostream>
#include <chrono>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemkv.h>
#include <string>
#include <cassert>

#define PAGE_SIZE 4096
#define PMEM_SIZE 100*1024*1024*1024UL

pmemkv_db *db=NULL;

pmemkv_config* config_setup(const char* path, const uint64_t fcreate, const uint64_t size) 
{
  pmemkv_config *cfg = pmemkv_config_new();
  assert(cfg != nullptr);
	
  if (pmemkv_config_put_string(cfg, "path", path) != PMEMKV_STATUS_OK) {
    fprintf(stderr, "%s", pmemkv_errormsg());
    return NULL;
  }
	
  if (pmemkv_config_put_uint64(cfg, "force_create", fcreate) != PMEMKV_STATUS_OK) {
	fprintf(stderr, "%s", pmemkv_errormsg());
	return NULL;
  }
	
  if (pmemkv_config_put_uint64(cfg, "size", size) != PMEMKV_STATUS_OK) {
	fprintf(stderr, "%s", pmemkv_errormsg());
	return NULL;
  }
	
  return cfg;
}

//初始化，传入的是文件的名称，文件名称也可以定义在头文件中间。
int cbs_init(const char * filename)
{
  int s;
  pmemkv_config *cfg=NULL;
  if(access(filename,F_OK)==0) {
    cfg = config_setup(filename, 0,PMEM_SIZE);
  } else {
    cfg = config_setup(filename, 1,PMEM_SIZE);
  }
  assert(cfg != NULL);
  
  //cmap open with the cfg.  
  s = pmemkv_open("cmap", cfg, &db);
  assert(s == PMEMKV_STATUS_OK);
  assert(db != NULL);
  return 0;
}

//写入或者更新一个页，输入req_id表示更新到什么地方，content表示更新的内容
int write_req(uint64_t req_id, unsigned char * content) {
  int s;
  char str[20];
  //itoa(req_id,string, 10); 
  sprintf(str,"%ld",req_id);
  s = pmemkv_put(db, str, strlen(str), (const char *)content, PAGE_SIZE);
  assert(s==PMEMKV_STATUS_OK);
  return 0;
}

size_t get_key_count() 
{
  size_t cnt;
  int s;

  s=pmemkv_count_all(db,&cnt);
  assert(s == PMEMKV_STATUS_OK);
  return cnt;
}

void pmemkv_get_value(const char *value, size_t valuebytes, void *val) {
  //printf("0x%x\n",value[0]);
  //printf("%ld\n",valuebytes);
  memcpy((char *)val, value,valuebytes);
  return;
}
		   
unsigned char * read_req(uint64_t req_id,void *val) {
  int s;
  char str[20];
  sprintf(str,"%ld",req_id);
  //val is a heap with 4096. 
  s = pmemkv_get(db, str, strlen(str),pmemkv_get_value, val);
  assert(s == PMEMKV_STATUS_OK);
  return (unsigned char *)val;
}

void delete_page(uint64_t req_id) {
  return;
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

  unsigned char * read_content;
  memset(page_content,0xab,4096);
  
  start=std::chrono::steady_clock::now();
  cbs_init("/mnt/pmem0/cbs_pmemkv");
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"cbs_init time"<<diff.count()<<std::endl; 
  size_t cnt=get_key_count();
  std::cout<<"kv count"<<cnt<<std::endl; 

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
    read_content=read_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite read_req take time "<<diff.count()/OVERWRITE_COUNT<<std::endl;
  printf("the page should fill with paten 0xcd, 0x%x\n", page_content[0]);

  start = std::chrono::steady_clock::now();
  for(i=OVERWRITE_COUNT;i<WRITE_COUNT;i++) {
    read_content=read_req(i,page_content);
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

