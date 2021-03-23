#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libpmem.h>
#include <time.h>
#include <string.h>
#include <cassert>

//使用libpmeobj来保存所有的page cache
#include <libpmemobj.h>

#define PAGE_SIZE 4096
#define BLOCK_PAGE_NUM 1024
#define REQ_BLOCK 10
#define REQ_PAGE 0x3ff
#define DISK_BLOCK_NUM 1048576 //1024*1024 blocks 
#define PMEM_SIZE 100*1024*1024*1024UL  //定义我们使用持久内存的大小

/****************************************
 * \
 * pmem layout as:
 --block_oid[0]--|--block_oid[1]--|...|--[[[page_oid][page_oid]...]cached_page_cnt]--|
rebuild the block dram structure after the system restart. 
 */
//cbs cache layout in pmem
#define LAYOUT_NAME "cbs_cache"
POBJ_LAYOUT_BEGIN(cbs_cache);
POBJ_LAYOUT_ROOT(cbs_cache, struct root);
POBJ_LAYOUT_END(cbs_cache);

//block conents have the BLOCK_PAGE_NUM, pages. 
struct page {
  unsigned char rpage[PAGE_SIZE];
};

struct block { /* array-based queue container */
  PMEMoid  pages[BLOCK_PAGE_NUM];
  uint64_t cached_page_cnt;
};
//root object max size is 15G, defined in the head file. root size is limited, so at the poc, we just se the BLOCK NUM as 1000;
struct root {
  PMEMoid blocks[DISK_BLOCK_NUM];
};

struct root * rootp=NULL;
PMEMobjpool *pop=NULL;

//初始化，传入的是文件的名称，文件名称也可以定义在头文件中间。
int cbs_init(const char * filename)
{
  //printf("cbs_init, the filename=%s\n",filename);
  pop = pmemobj_create(filename, LAYOUT_NAME,
                                PMEM_SIZE, 0666);
  if (pop != NULL) {
    printf("pmemobj_create successfully\n");
  } else {
    pop = pmemobj_open(filename, POBJ_LAYOUT_NAME(cbs_cache));
    if (pop == NULL) {
      printf("failed to open the pool\n");
      return 1;
    }
  }

  PMEMoid root = pmemobj_root(pop, sizeof(struct root));
  rootp = (struct root *)pmemobj_direct(root);
  if(rootp==NULL) {
    printf("rootp is NULL, please check your persistent memory pool\n");
    return 1;
  }
  return 0;
}

int get_cached_count() {
  int i;
  int cnt;
  PMEMoid block_oid;
  struct block * blockp=NULL;
  for (i=0;i<DISK_BLOCK_NUM;i++) {
    block_oid=rootp->blocks[i];
    if(!OID_IS_NULL(block_oid)) {
      blockp=(struct block *) pmemobj_direct(block_oid);
      cnt+=blockp->cached_page_cnt;
    }
  }
  return cnt;
}

//写入或者更新一个页，输入req_id表示更新到什么地方，content表示更新的内容
int write_req(uint64_t req_id, unsigned char * content) {
  //req_id>>10是block_id(1024 pages/block); req_id&0x3ff是该block中的page_id.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  assert(req_page_id<BLOCK_PAGE_NUM); //req id and req number in one block, can't over 1024; req id should not duplicate.
  TX_BEGIN(pop) {
    PMEMoid block_oid = rootp->blocks[block_id];   //block_oid,找到相应的block
    
    if(OID_IS_NULL(block_oid)) { //if block_oid is NULL，我们先分配相应的block还有相应的page
      block_oid = pmemobj_tx_alloc(sizeof(struct block), 0);
      struct block * blockp=(struct block *) pmemobj_direct(block_oid);
      assert(blockp!=NULL);
      memset(blockp,0x0,sizeof(struct block));
      PMEMoid page_oid = pmemobj_tx_alloc(sizeof(struct page), 0);
      struct page * pagep = (struct page *) pmemobj_direct(page_oid);
      assert(pagep!=NULL);
      memcpy(pagep->rpage,content,PAGE_SIZE);

      //rootp->blocks[block_id] update
      pmemobj_tx_add_range_direct(&rootp->blocks[block_id],sizeof(PMEMoid));
      rootp->blocks[block_id]=block_oid;
      
      pmemobj_tx_add_range_direct(&blockp->pages[req_page_id],sizeof(PMEMoid));
      blockp->pages[req_page_id]=page_oid;

      pmemobj_tx_add_range_direct(&blockp->cached_page_cnt,sizeof(uint64_t));
      blockp->cached_page_cnt++;
    } else {  //block已经在持久内存中了
      struct block * blockp=(struct block *) pmemobj_direct(block_oid);
      PMEMoid page_oid = blockp->pages[req_page_id];
      if(OID_IS_NULL(page_oid)) {//如果相对应的page还没有分配，分配page 
  	page_oid = pmemobj_tx_alloc(sizeof(struct page), 0);
 	struct page * pagep= (struct page *)pmemobj_direct(page_oid);
	assert(pagep!=NULL);
	memcpy(pagep->rpage,content,PAGE_SIZE);
	// update need to tx_add snapshot;
	pmemobj_tx_add_range_direct(&blockp->pages[req_page_id],sizeof(PMEMoid));
	blockp->pages[req_page_id]=page_oid;

	pmemobj_tx_add_range_direct(&blockp->cached_page_cnt,sizeof(uint64_t));
	// 8 bytes, no need to snapshot. 
	blockp->cached_page_cnt++;
      }else { //如果已经分配了page，将原有page的内容填入undo log； 通过pmemobj_tx_add_range_direct(pagep->rpage,PAGE_SIZE);
        struct page * pagep= (struct page *)pmemobj_direct(page_oid);
 	assert(pagep!=NULL);
 	pmemobj_tx_add_range_direct(pagep->rpage,PAGE_SIZE);
 	memcpy(pagep->rpage, content,PAGE_SIZE);
      }		  
    }
  }TX_END

  return 0;
}

void * read_req(uint64_t req_id) {
  // req_id and content; only after the init success, then this API can be called.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  PMEMoid block_oid = rootp->blocks[block_id]; 
  if(OID_IS_NULL(block_oid)) {
	return NULL;
  }else {
    struct block * blockp=(struct block *) pmemobj_direct(block_oid);
	PMEMoid page_oid = blockp->pages[req_page_id];
	if(OID_IS_NULL(page_oid)) {
	  return NULL;
	}else {
	  struct page * pagep= (struct page *)pmemobj_direct(page_oid);
	  assert(pagep!=NULL);
	  return pagep->rpage;
	}
  }
}

void delete_page(uint64_t req_id) {
  // req_id and content; only after the init success, then this API can be called.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  PMEMoid block_oid = rootp->blocks[block_id]; 
  if(OID_IS_NULL(block_oid)) {
	return ;
  } else {
    struct block * blockp=(struct block *) pmemobj_direct(block_oid);
	PMEMoid page_oid = blockp->pages[req_page_id]; 
	if(OID_IS_NULL(page_oid)) {
	  return ;
	} else {
	  TX_BEGIN(pop) {
		pmemobj_tx_free(page_oid);
	  }TX_END
	}
  }
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

  unsigned char * read_content;
  memset(page_content,0xab,PAGE_SIZE);
  
  start=std::chrono::steady_clock::now();
  cbs_init("/mnt/pmem0/cbs_cache");
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"cbs_init time"<<diff.count()<<std::endl; 
  
  std::cout<<"cached page count"<<get_cached_count()<<std::endl;

  start = std::chrono::steady_clock::now();
  for(i=0;i<WRITE_COUNT;i++) {
    write_req(i,page_content);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"write_req time"<<diff.count()/WRITE_COUNT<<std::endl;

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
    memcpy(page_content,read_content,PAGE_SIZE);
  }
  stop=std::chrono::steady_clock::now();
  diff=stop-start;
  std::cout<<"overwrite read_req take time "<<diff.count()/OVERWRITE_COUNT<<std::endl;
  printf("the page should fill with paten 0xcd, 0x%x\n", page_content[0]);

  start = std::chrono::steady_clock::now();
  for(i=OVERWRITE_COUNT;i<WRITE_COUNT;i++) {
    read_content=(unsigned char *)read_req(i);
    memcpy(page_content,read_content,PAGE_SIZE);
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

