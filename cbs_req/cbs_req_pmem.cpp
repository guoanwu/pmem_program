#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libpmem.h>
#include <time.h>
#include <string.h>

#define BLOCK_PAGE_NUM 1024
#define REQ_BLOCK 10
#define REQ_PAGE 0x3ff
#define DISK_BLOCK_NUM 1048576 //1024*1024 blocks 
#define MAGIC_NUMBER 0xabcdabcd
/****************************************
 * \
 * pmem layout as:
 ---8byte magic number---|--- 8byte page_meta[0]---|---8byte page_meta[1]---|...|---4k page[0]---|---4k page[1]---|
Page meta 和真正的4k页有对应关系。 
rebuild the block dram and freelist structure after the system restart. 
 */
//8 bytes atomic,必须一起更新，因为这三个域是一个原子更新。不能出现一个域更新了，而另外的域还没有更新的情况
typedef struct page_meta {
  uint64_t valid:1; //valid 表示该page是否已经分配和使用，如果是0，表示该page没有被使用
  uint64_t sn:31;   //sequence number来表示页的新旧，sequence越大表示页越新，old page一般sn会被重置为0
  uint64_t req_id:32;  //req_id, which is in the range from 1~1024*1024*1024
  uint64_t padding[7];
}page_meta_t;
//magic number，可以不定义这个数据结构，因为位置是固定的
typedef struct pmem_layout {
  uint64_t magic_number;
  uint64_t page_offset;    //real page offset，保证这个offset 4K对齐。
  uint64_t padding[6];
}pmem_layout_t;
//持久内存的基地址
char * pmem_base;
uint64_t page_address;

#define PAGE_SIZE 4096
#define PMEM_META_SIZE sizeof(page_meta_t)
#define MAGIC_NUM_SIZE sizeof(pmem_layout_t)
#define PMEM_SIZE 100*1024*1024*1024UL  //定义我们使用持久内存的大小100GB
#define PMEM_PAGE_NUMBERS ((PMEM_SIZE-MAGIC_NUM_SIZE)/(PAGE_SIZE+PMEM_META_SIZE)-1) //根据持久内存的大小，我们可以知道我们有多少个内存页
#define PAGE_ID_FROM_META(meta) (((uint64_t)meta-MAGIC_NUM_SIZE-(uint64_t)pmem_base)/PMEM_META_SIZE+1) //根据持久内存的page meta而知道page_id
#define PAGE_FROM_META(meta) (char *)(page_address + PAGE_SIZE * (((uint64_t)meta - MAGIC_NUM_SIZE- (uint64_t)pmem_base)/PMEM_META_SIZE)) //meta->real page
#define PAGE_META_FROM_ID(id) (page_meta_t *) ((uint64_t)pmem_base+MAGIC_NUM_SIZE+(id-1)*PMEM_META_SIZE) //从pageid知道page meta

/*****************************************
 * *
 * disk structure,这个数据结构放在内存中，在初始化的时候扫描page meta数据，恢复disk内存数据结构
 * [[page_id1,page_id2]* BLOCK_PAGE_NUM],cache_pages_cnt]* DISK_BLOCK_NUM
 */
typedef struct block_page {
  uint64_t page_id1:32;
  uint64_t page_id2:32;
}block_page_t;
typedef struct block_data {
  block_page_t pages[BLOCK_PAGE_NUM];
  uint32_t cached_pages_cnt;  //if the count over one threshold, might need to write back to the SSD.   
}block_data_t;
typedef struct disk {
  block_data_t blocks[DISK_BLOCK_NUM];
}disk_t;
disk_t * disk;

//a free list that always pick the free page from the free_list[free_num-1];
//初始化的时候扫描page meta数据，恢复free pages的数据结构
typedef struct free_pages{
  uint64_t free_num;
  page_meta_t ** free_list;
}free_pages_t;
free_pages_t * free_pages;

//初始化，传入的是文件的名称，文件名称也可以定义在头文件中间。
int cbs_init(const char * filename)
{
  size_t mapped_len, pagecache_num;
  int is_pmem;
  int i;
  //将持久内存映射到虚拟地址空间并得到基地址pmem_base
  if ((pmem_base = (char *)pmem_map_file(filename, PMEM_SIZE,
                                            PMEM_FILE_CREATE, 0666,
                                            &mapped_len, &is_pmem)) == NULL) {
    perror("Pmem map file failed");
    return -1;
  }
  printf("pmem_map_file mapped_len=%ld, pmem_base=%p, is_pmem=%ld\n", mapped_len, pmem_base, is_pmem);
  pmem_layout_t * pmem_data =(pmem_layout_t *) pmem_base;
  page_meta_t * pmem_meta = (page_meta_t *)((uint64_t)pmem_base+sizeof(pmem_layout_t));
  
  //分配内存数据结构
  disk=(disk_t *)calloc(sizeof(char),sizeof(disk_t));
  if(disk == NULL) return -1;

  free_pages = (free_pages_t*)(malloc(sizeof(free_pages_t)));
  free_pages->free_list =(page_meta_t **) malloc(PMEM_PAGE_NUMBERS*sizeof(page_meta_t *));
  if(free_pages == NULL || free_pages->free_list == NULL) {
    printf("DRAM space is not enough for store the free_list\n");
    return -1;
  }
  
  //check magic number，如果magic number已经写过了，那可以恢复disk数据结构，否则所有的页都是free的，建立free pages数据结构
  if(pmem_data->magic_number != MAGIC_NUMBER) {
    //first time init and write the whole block structure to 0
    //pmem_memset_persist(pmem_base,0x00,PMEM_SIZE);
    pmem_memset_persist(pmem_meta,0x0,sizeof(page_meta_t)*PMEM_PAGE_NUMBERS);
    page_address = (uint64_t)(((uint64_t )pmem_meta + sizeof(page_meta_t)*PMEM_PAGE_NUMBERS + PAGE_SIZE) & 0xfffffffffffff000);
    pmem_data->page_offset = page_address - (uint64_t)pmem_base;
    pmem_persist(&(pmem_data-> page_offset),sizeof(pmem_data-> page_offset));
    
    pmem_data->magic_number = MAGIC_NUMBER;
    pmem_persist(pmem_data,sizeof(pmem_data->magic_number));
    
    for(i=0;i<PMEM_PAGE_NUMBERS;i++) {
      free_pages->free_list[i]=pmem_meta;
      pmem_meta+=1;
    }
    free_pages->free_num=PMEM_PAGE_NUMBERS;
  }
  else {
    int j=0;
    uint64_t block_id,page_id;
    uint64_t req_id;
    //magic number check pass, that means we might have free pages caches. 
    for(i=0;i<PMEM_PAGE_NUMBERS;i++) {
      if(pmem_meta->valid == 0) {
        free_pages->free_list[j]=pmem_meta;
        pmem_meta+=1;
        j++;
      } else {
        //fill the disk structure.
        req_id = pmem_meta->req_id;
        block_id = req_id >> 10;
        page_id = req_id & 0x3ff;
        if(disk->blocks[block_id].pages[page_id].page_id1 == 0) {
          disk->blocks[block_id].pages[page_id].page_id1 = PAGE_ID_FROM_META(pmem_meta);
          disk->blocks[block_id].cached_pages_cnt++;
        }else {
          disk->blocks[block_id].pages[page_id].page_id1 = PAGE_ID_FROM_META(pmem_meta);
        }
        pmem_meta +=1;
      }
    }
    free_pages->free_num=j;
    page_address=pmem_data->page_offset + (uint64_t) pmem_base;
  }
  printf("init done, pagecache_num=%d,free page number=%d\n",PMEM_PAGE_NUMBERS, free_pages->free_num);
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
  uint64_t atomic_value=0;
  uint64_t page_id1, page_id2;
  uint64_t free_num=free_pages->free_num;

  if(block_id > DISK_BLOCK_NUM) {
    printf(" write req_id is not valid and over the disk range\n");
    return -1;
  }
  page_id1=disk->blocks[block_id].pages[req_page_id].page_id1;
  page_id2=disk->blocks[block_id].pages[req_page_id].page_id2;
  page_meta_t ** free_list = free_pages->free_list;
  if(free_num ==0 ) {
    printf("thre is no free pages in the pmem\n");
  }

  if(page_id1==0 && page_id2==0) { 
    //there is no page in the location, add one page; Get one free page meta in the free_list
    page_meta_t * page_new = free_list[free_num-1];
    pmem_memcpy_persist(PAGE_FROM_META(page_new), content, 4096);  //从pagemeta中拿到4k page，将数据写入并持久化
    atomic_value = req_id<<32|1;
    pmem_memcpy_persist(page_new,&atomic_value,8); //更新page meta valid=1，sn=0， req_id，atomic写入
    
    //更新内存中的数据结构，free number减一， block中page_id1更新，cached_page_cnt加一
    free_pages->free_num-=1;
    disk->blocks[block_id].pages[req_page_id].page_id1=PAGE_ID_FROM_META(page_new);
    disk->blocks[block_id].cached_pages_cnt++;
  } 
  else if((page_id1!=0 && page_id2==0)||(page_id1==0 && page_id2!=0)) {
    //已经有一个数据，此时需要更新，不能原地更新，必须先写道新的位置上
    page_meta_t * page_old;
    if(page_id1!=0) {
      page_old=PAGE_META_FROM_ID(page_id1);
    }else {
      page_old=PAGE_META_FROM_ID(page_id2);    
    }
    if(page_old->sn!=0) {
      page_old->sn=0;
      pmem_persist(page_old,8);  //老的页先把sn reset为0
    }
    //找到一个新的page meta和4kpage，先将4k数据写入，如果写入过程断电，由于page_meta不会更新，所以恢复不会出错。
    page_meta_t * page_new = free_list[free_num-1];
    pmem_memcpy_persist(PAGE_FROM_META(page_new), content, 4096);
    atomic_value = req_id<<32|1<<1|1;  //sn=1， req_id, valid=1, 原子写入
    pmem_memcpy_persist(page_new,&atomic_value, 8);
    disk->blocks[block_id].pages[req_page_id].page_id2=PAGE_ID_FROM_META(page_new);

    //如果此时断电，在这个req_id上有两个页都是有效的，其中sn=0的是老的数据，sn=1的是新的数据。
    //将老的数据free，将老的meta page_old写入0， sn=0；valid=0，req_id=0;    
    atomic_value=0;
    pmem_memcpy_persist(page_old,&atomic_value, 8);
    free_list[free_num-1]=page_old;
    disk->blocks[block_id].pages[req_page_id].page_id1=0;
  }  
  else if(page_id1!=0 && page_id2!=0) {  
    // 如果在这个req_id上两个页都是有效的，检查sn，sn越大的数据越新。
    page_meta_t * page1_meta, *page2_meta;    
    page1_meta=PAGE_META_FROM_ID(page_id1);
    page2_meta=PAGE_META_FROM_ID(page_id2);
    if(page1_meta->sn < page2_meta->sn) { //page_id2是新的数据
      //更新page_id1,让page_id1对应的数据更新
      pmem_memcpy_persist(PAGE_FROM_META(page1_meta), content, 4096);
      atomic_value = req_id<<32|(page2_meta->sn++)<<1|1;
      pmem_memcpy_persist(page1_meta,&atomic_value, 8);
      
      atomic_value=0; //释放page_id2对应得数据
      pmem_memcpy_persist(page2_meta,&atomic_value, 8);
      disk->blocks[block_id].pages[req_page_id].page_id2=0;
      free_num++;
      free_pages->free_num=free_num;
      free_list[free_num-1]=page2_meta;
    } else {
      pmem_memcpy_persist(PAGE_FROM_META(page2_meta), content, 4096);
      atomic_value = req_id<<32|(page1_meta->sn++)<<1|1;
      pmem_memcpy_persist(page2_meta,&atomic_value, 8);
      
      atomic_value=0;
      pmem_memcpy_persist(page2_meta,&atomic_value, 8);
      disk->blocks[block_id].pages[req_page_id].page_id1=0;
      free_num++;
      free_pages->free_num=free_num;
      free_list[free_num-1]=page1_meta;
    }
  }
  return 0;
}

void * read_req(uint64_t req_id) {
  // req_id and content; only after the init success, then this API can be called.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  uint64_t atomic_value=0;
  uint64_t page_id1, page_id2;
  if(block_id > DISK_BLOCK_NUM) {
    printf("read req_id is not valid and over the disk range\n");
    return NULL;
  }

  page_id1=disk->blocks[block_id].pages[req_page_id].page_id1;
  page_id2=disk->blocks[block_id].pages[req_page_id].page_id2;
  
  if((page_id1!=0 && page_id2 == 0) ||(page_id1==0 && page_id2!=0)) { // only page1 is there 
    page_meta_t * page_meta;
    if(page_id1!=0) {
      page_meta=PAGE_META_FROM_ID(page_id1);
    }else {
      page_meta=PAGE_META_FROM_ID(page_id2);
    }
    return PAGE_FROM_META(page_meta);
    
  } else if(page_id1!=0 && page_id2!=0) { // two pages 
    page_meta_t * page1_meta=PAGE_META_FROM_ID(page_id1);
    page_meta_t * page2_meta=PAGE_META_FROM_ID(page_id2);
    if(page1_meta->sn < page2_meta->sn) {
      return PAGE_FROM_META(page2_meta);
    } else {
      return PAGE_FROM_META(page1_meta);
    }
  } else {
    printf("cache is not exist\n");
  }

  return NULL;
}

void delete_page(uint64_t req_id) {
  // req_id and content; only after the init success, then this API can be called.
  uint64_t block_id = req_id >> REQ_BLOCK;
  uint64_t req_page_id = req_id & REQ_PAGE;
  uint64_t atomic_value=0;
  uint64_t page_id1, page_id2;
  if(block_id > DISK_BLOCK_NUM) {
    printf("read req_id is not valid and over the disk range\n");
    return ;
  }
  page_id1=disk->blocks[block_id].pages[req_page_id].page_id1;
  page_id2=disk->blocks[block_id].pages[req_page_id].page_id2;

  uint64_t free_num=free_pages->free_num;
  page_meta_t ** free_list = free_pages->free_list;
  if(free_num ==0 ) {
    printf("there is no free pages in the pmem\n");
  }
  atomic_value=0;
  if((page_id1!=0 && page_id2 == 0) ||(page_id1==0 && page_id2!=0)) { // only one page is there 
    page_meta_t * page_meta;
    if(page_id1!=0) {
      page_meta=PAGE_META_FROM_ID(page_id1);
      disk->blocks[block_id].pages[req_page_id].page_id1=0;
    }else {
      page_meta=PAGE_META_FROM_ID(page_id2);
      disk->blocks[block_id].pages[req_page_id].page_id2=0;
    }
    pmem_memcpy_persist(page_meta,&atomic_value, 8);
    free_num++;
    free_pages->free_num=free_num;
    free_list[free_num-1]=page_meta;
  } else if(page_id1!=0 && page_id2!=0) { // two pages 
    page_meta_t * page1_meta=PAGE_META_FROM_ID(page_id1);
    page_meta_t * page2_meta=PAGE_META_FROM_ID(page_id2);
    if(page1_meta->sn < page2_meta->sn) {
      pmem_memcpy_persist(page1_meta,&atomic_value, 8);
      pmem_memcpy_persist(page2_meta,&atomic_value, 8);
    } else {
      pmem_memcpy_persist(page2_meta,&atomic_value, 8);    
      pmem_memcpy_persist(page1_meta,&atomic_value, 8);
    }
    disk->blocks[block_id].pages[req_page_id].page_id1=0;
    disk->blocks[block_id].pages[req_page_id].page_id2=0;
    free_num++;
    free_list[free_num-1]=page1_meta;
    free_num++;
    free_list[free_num-1]=page2_meta;
    free_pages->free_num=free_num;
  } else {
    printf("cache is not exist\n");
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
  cbs_init("/mnt/pmem0/cbs_file");
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

