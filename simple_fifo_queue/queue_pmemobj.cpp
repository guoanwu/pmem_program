#include <libpmemobj.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

#define CREATE_MODE_RW (S_IWRITE | S_IREAD)
POBJ_LAYOUT_BEGIN(pmem_que);
POBJ_LAYOUT_ROOT(pmem_que, struct queue);
POBJ_LAYOUT_TOID(pmem_que, struct queue_node);
POBJ_LAYOUT_END(pmem_que);

/*
 *  * file_exists -- checks if file exists
 *   */
static inline int
file_exists(char const *file)
{
        return access(file, F_OK);
}

typedef enum que_op{
  ENQUE,
  DEQUE,
  SHOW,
  MAX_OPS,
}que_op_e;

struct queue_node {
  int value;
  TOID(struct queue_node)  next; 
};

struct queue {
  TOID(struct queue_node) head;
  TOID(struct queue_node) tail;  	
};


const char * ops_str[MAX_OPS]={"enque","deque","show"};
que_op_e parse_que_ops(char * ops) 
{
  int i;
  for(i=0;i<MAX_OPS;i++) 
  {
    if(strcmp(ops_str[i],ops)==0) {
    	return (que_op_e)i;
    }
  }
  return MAX_OPS;
}

TOID(struct queue) pmem_queue;

void queue_enque(PMEMobjpool *pop, int value)
{
  TX_BEGIN(pop) {
    TOID(struct queue_node) head=D_RW(pmem_queue)->head;
    TOID(struct queue_node) tail=D_RW(pmem_queue)->tail;
    
    TOID(struct queue_node) newnode= TX_NEW(struct queue_node);
    D_RW(newnode)->value=value;
    D_RW(newnode)->next=TOID_NULL(struct queue_node);    

    if(TOID_IS_NULL(head)) {
      TX_ADD(pmem_queue);	
      D_RW(pmem_queue)->head=newnode;
      D_RW(pmem_queue)->tail=newnode;
    }
    else 
    {
      //TX_ADD(tail);
      TX_ADD_DIRECT(&(D_RW(tail)->next));
      D_RW(tail)->next=newnode;	
      //TX_ADD(pmem_queue);
      TX_ADD_DIRECT(&(D_RW(pmem_queue)->tail));
      D_RW(pmem_queue)->tail=newnode;
    }  
    //return;
  }TX_END 
}

int queue_deque(PMEMobjpool *pop,int * value)
{
  TX_BEGIN(pop) {
    TOID(struct queue_node) head=D_RW(pmem_queue)->head;
    TOID(struct queue_node) tail=D_RW(pmem_queue)->tail;
    
    if(TOID_IS_NULL(head)) {
      return -1;
    }   
    else
    {
      *value=D_RO(head)->value;
      
      //TX_ADD(pmem_queue);
      TX_ADD_DIRECT(&(D_RW(pmem_queue)->head));
      D_RW(pmem_queue)->head=D_RO(head)->next;

      if(TOID_EQUALS(head,tail)) {
	 TX_ADD_DIRECT(&(D_RW(pmem_queue)->tail));
         D_RW(pmem_queue)->tail=D_RO(tail)->next;
      
      }
      TX_FREE(head);
    }
  }TX_END
  return 0;
}

int queue_show() 
{
  TOID(struct queue_node) head=D_RO(pmem_queue)->head;
  if(TOID_IS_NULL(head)) return -1;
   
  while(!TOID_IS_NULL(head)) 
  {
    std::cout<<D_RO(head)->value<<std::endl;
    head=D_RO(head)->next;
  }
  return 0;
}

int main(int argc, char * argv[])
{
  if(argc<3) {
    //printf("usage: %s filename [enque[value]|deque|show] value",argv[0]);
    std::cerr << "usage: " << argv[0] << " file-name [enque [value]|deque|show]" << std::endl;
    exit(0);
  }
  const char * path=argv[1];
  // before pmemobj create, if you see pmemobj_create failed, use the following two lines 
  int sds_write_value = 0;
  pmemobj_ctl_set(NULL, "sds.at_create", &sds_write_value);
 
  //PMEMobjpool *pmemobj_create(const char *path, const char *layout,
  //                  size_t poolsize, mode_t mode); 
  
  PMEMobjpool *pop;
  if (file_exists(path) != 0) {
    pop=pmemobj_create(path,POBJ_LAYOUT_NAME(pmem_que),PMEMOBJ_MIN_POOL,CREATE_MODE_RW);
  } else {
    pop=pmemobj_open(path,POBJ_LAYOUT_NAME(pmem_que));
  }
  
  pmem_queue= POBJ_ROOT(pop, struct queue);
  
  //parse string
  que_op_e ops=parse_que_ops(argv[2]);
  switch(ops) {
    case ENQUE:
      queue_enque(pop,atoi(argv[3]));
      break;
    case DEQUE:
      int val;
      if(queue_deque(pop, &val)!=-1) 
      {
        std::cout<<"deque:"<<val<<std::endl;
      }
      break;
    case SHOW:
      queue_show();
      break;
    case MAX_OPS:
      std::cerr << "unknown ops"<<std::endl;
      break;
  }
  
  pmemobj_close(pop);
}
