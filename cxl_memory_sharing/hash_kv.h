#ifndef __HASH_KV_H__
#define __HASH_KV_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 256

// 链表节点
typedef struct Node {
    long int key;
    long int value;
    struct Node* next;
} Node;

// 哈希表
typedef struct {
    Node* list[TABLE_SIZE];
} HashTable;

// 创建哈希表
HashTable* createHashTable();
// 设置键值对
void setKeyValue(HashTable* table, long int key, long int value);
// 获取键对应的值
long int getKeyValue(HashTable* table, long int key); 
// 释放哈希表内存
void freeHashTable(HashTable* table);

#endif



