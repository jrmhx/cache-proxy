#ifndef COMP2310_CACHE_H
#define COMP2310_CACHE_H

#include <stdlib.h>
#include "csapp.h"

typedef struct cache_block {
    int freq; // frequency of access
    int port;
    char hostname[MAXLINE];
    char path[MAXLINE];
    char *content; // the content of the cache block (the response)
    size_t size; // the size of the content
    struct cache_block *prev; // the prev cache block
    struct cache_block *next; // the next cache block
    pthread_rwlock_t block_lock;
} cache_block;

typedef struct cache { // the cache is a double linked list
    struct cache_block *head; // the head of the cache
    struct cache_block *tail; // the tail of the cache
    size_t c_size; // the total size of the cache
    pthread_rwlock_t cache_lock; // the lock of the cache
} Cache;


void cache_init(void);
void cache_deinit(void);
void cache_insert_LRU(char* hostname, char *path, int port, char *content, size_t size);
cache_block *cache_find_LRU( char* hostname, char *path, int port);
void cache_delete_LRU(void);
void print_cache(void);

void cache_insert_LFU(char *uri, char *content, size_t size);
char *cache_find_LFU(char *uri);
void cache_delete_LFU(void);



#endif //COMP2310_CACHE_H
