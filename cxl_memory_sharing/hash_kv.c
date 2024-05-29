#include "hash_kv.h"

// 简单的哈希函数
static unsigned long int hashFunction(long int key) {
    return key % TABLE_SIZE;
}

HashTable* createHashTable() {
    HashTable* table = (HashTable *)malloc(sizeof(HashTable));
    if (!table) {
        return NULL;
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        table->list[i] = NULL;
    }
    return table;
}


// 设置键值对
void setKeyValue(HashTable* table, long int key, long int value) {
    if (!table) return;

    // 找到哈希索引
    unsigned long int index = hashFunction(key);

    // 遍历链表，如果key已存在则更新
    Node* node = table->list[index];
    while (node != NULL) {
        if (node->key == key) {
            node->value = value;
            return;
        }
        node = node->next;
    }

    // 如果key不存在，创建新节点并插入链表头部
    Node* new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        return; // 内存分配失败
    }
    new_node->key = key;
    new_node->value = value;
    new_node->next = table->list[index];
    table->list[index] = new_node;
}

// 获取键对应的值
long int getKeyValue(HashTable* table, long int key) {
    if (!table) return -1; // 空表

    unsigned long int index = hashFunction(key);
    Node* node = table->list[index];

    // 遍历链表查找键
    while (node != NULL) {
        if (node->key == key) {
            return node->value;
        }
        node = node->next;
    }

    return -1; // 键未找到
}

// 释放哈希表内存
void freeHashTable(HashTable* table) {
    if (!table) return;

    for (int i = 0; i < TABLE_SIZE; i++) {
        Node* node = table->list[i];
        while (node != NULL) {
            Node* temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(table);
}

/*
int main() {
    HashTable* myHashTable = createHashTable();
    if (!myHashTable) {
        printf("Failed to create hash table.\n");
        return 1;
    }

    // 设置一些键值对
    setKeyValue(myHashTable, 10, 100);
    setKeyValue(myHashTable, 20, 200);
    setKeyValue(myHashTable, 30, 300);

    // 获取并打印键对应的值
    long int value = getKeyValue(myHashTable, 20);
    if (value != -1) {
        printf("The value for key 20 is: %ld\n", value);
    }

    // 释放哈希表
    freeHashTable(myHashTable);

    return 0;
}
*/
