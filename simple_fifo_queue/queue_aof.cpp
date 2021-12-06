#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <libpmem.h>
typedef enum que_op{
  ENQUE,
  DEQUE,
  SHOW,
  PREQUE,
  MAX_OPS,
}que_op_e;

typedef struct queue_node {
  int value;
  struct queue_node *  next; 
}queue_node_t;

typedef struct queue {
  queue_node_t * head;
  queue_node_t * tail;  	
}queue_t;


const char * ops_str[MAX_OPS]={"enque","deque","show","preque"};
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

static queue_t * pmem_queue = NULL;

void queue_enque_v(int value)
{
  queue_node_t * node= (queue_node_t *)malloc(sizeof(queue_node_t));
  node->value=value;
  node->next = NULL;
  
  if(pmem_queue == NULL) 
  {
    pmem_queue= (queue_t *) malloc(sizeof(queue_t));
    pmem_queue->head=node;
    pmem_queue->tail=node;
  } 
  else 
  {
    queue_node_t * tail = pmem_queue->tail;
    tail->next=node;
    pmem_queue->tail = node;
  }  
}
//volatile version
int queue_deque_v(int * value)
{
  if(pmem_queue == NULL) return -1;
    
  queue_node_t * head = pmem_queue->head;
  queue_node_t * tail = pmem_queue->tail;
  if(head == tail) pmem_queue->tail=NULL;
  pmem_queue->head=head->next;

  *value=head->value;
  free(head);
  return 0;
}

int queue_show() 
{
  if(pmem_queue==NULL) return -1;
  queue_node_t * node;
  node=pmem_queue->head;
  while(node!=NULL) 
  {
    //printf("");
    std::cout<<" "<<node->value;
    node=node->next;
  }
  std::cout<<std::endl;
  return 0;
}

int main(int argc, char * argv[])
{
  char command[32];
  int value;
  //add the append of log for persistent.
  if(argc <2) {
    std::cout<<"usage: %s aoflogfilename"<<argv[0]<<std::endl;
    exit(0);
  }
  char * filename=argv[1];
  FILE * fp = fopen(filename,"a+");
  char buf[32];
  const char *sep=" ";
  char * p;
  
  while(fgets(buf, sizeof(buf), fp))
  {
      //printf("%s\n", buf);
      p= strtok(buf,sep);
      que_op_e ops=parse_que_ops(p);
      if(ops == ENQUE) {
	    p=strtok(NULL,sep);
        queue_enque_v(atoi(p));
      } 
      else if(ops == DEQUE) {
     	queue_deque_v(&value);
      }
  }
  //fclose(fp);
  while(1) 
  {
    std::cout<<"input command:enque|deque|show|preque "<<std::endl; 
    scanf("%s", command);
    //parse string
    que_op_e ops=parse_que_ops(command);
    switch(ops) {
      case ENQUE:
        std::cout<<"enque [value]"<<std::endl;
        scanf("%d",&value);
        queue_enque_v(value);
        // record to the aof log
        sprintf(buf,"enque %d\n",value);
        fwrite(buf,strlen(buf),1,fp);
        fflush(fp);
	fsync(fileno(fp));
        break;
      case DEQUE:
        int val;
        if(queue_deque_v(&val)!=-1) 
        {
          std::cout<<"deque:"<<val<<std::endl;
        }
	sprintf(buf,"deque\n");
	fwrite(buf,strlen(buf),1,fp);
	fflush(fp);
	fsync(fileno(fp));
        break;
      case SHOW:
        queue_show();
        break;
      case PREQUE:
        for(value=0;value<10000;value++) {
          queue_enque_v(value);
          // record to the aof log
          sprintf(buf,"enque %d\n",value);
          fwrite(buf,strlen(buf),1,fp);
          fflush(fp);
          fsync(fileno(fp));
        }
        std::cout<<"prepare the queue and write into the AOF done"<<std::endl;
	break;
      case MAX_OPS:
        std::cerr << "unknown ops"<<std::endl;
        fclose(fp); 	
        exit(0);
    }
  }
}
