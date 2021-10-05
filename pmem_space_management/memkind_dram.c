#include <stdio.h>
#include <stdint.h>
#include <memkind.h>

//only need overwrite two function
void *malloc(size_t size) {
  return  memkind_malloc(MEMKIND_DEFAULT,size);
}
void free(void *ptr) {
  return memkind_free(MEMKIND_DEFAULT,ptr);
}

void *realloc(void * ptr, size_t size) {
  return memkind_realloc(MEMKIND_DEFAULT,ptr, size);
}

void *calloc(size_t num, size_t size) {
  return memkind_calloc(MEMKIND_DEFAULT,num,size);
}
