#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static Cache *cache;

void cache_init(void) {
    cache = Malloc(sizeof(Cache));
    cache->head = Malloc(sizeof(cache_block));
    //printf("%p\n", cache->head);
    cache->tail = Malloc(sizeof(cache_block));
    //printf("%p\n", cache->tail);
    //initialize the dummy head and tail
    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    cache->head->prev = NULL;
    cache->tail->next = NULL;
    cache->c_size = 0;
    pthread_rwlock_init(&cache->cache_lock, NULL);
}

void cache_deinit(void) {
    cache_block *temp = cache->head;
    while (temp != NULL) {
        cache->head = temp->next;
        if (temp->content != NULL) {
            Free(temp->content);
        }
        pthread_rwlock_destroy(&temp->block_lock);
        Free(temp);
        temp = cache->head;
    }
    pthread_rwlock_destroy(&cache->cache_lock);
    Free(cache);
}

cache_block *cache_find_LRU(char* hostname, char *path, int port)
{
    pthread_rwlock_rdlock(&cache->cache_lock);
    //printf("%p %p\n", cache, cache->head);
    cache_block *temp = cache->head->next;
    
    printf("test---------test\n");
    while (temp != cache->tail) {
        if (strcmp(temp->hostname, hostname) == 0 && strcmp(temp->path, path) == 0 && temp->port == port) {
            //move the block to the head
            pthread_rwlock_unlock(&cache->cache_lock);
            pthread_rwlock_wrlock(&cache->cache_lock);

            pthread_rwlock_wrlock(&temp->block_lock);
            temp->freq = temp->freq + 1;  
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;
            temp->next = cache->head->next;
            temp->prev = cache->head;
            pthread_rwlock_unlock(&temp->block_lock);

            cache->head->next->prev = temp;
            cache->head->next = temp;
            
            pthread_rwlock_unlock(&cache->cache_lock);
            return temp;
        }
        temp = temp->next;
    }
    pthread_rwlock_unlock(&cache->cache_lock);
    return NULL;
}

void cache_insert_LRU(char* hostname, char *path, int port, char *content, size_t size)
{
    pthread_rwlock_wrlock(&cache->cache_lock);
    cache_block *temp = Malloc(sizeof(cache_block));
    pthread_rwlock_init(&temp->block_lock, NULL);
    //temp->hostname = Malloc(strlen(hostname) + 1);
    strcpy(temp->hostname, hostname);
    //temp->path = Malloc(strlen(path) + 1);
    strcpy(temp->path, path);
    temp->port = port;
    temp->content = Malloc(size);
    memcpy(temp->content, content, size);
    temp->size = size;
    temp->freq = 0;
    temp->next = cache->head->next;
    temp->prev = cache->head;
    //insert the block to the head
    cache->head->next->prev = temp;
    cache->head->next = temp;
    cache->c_size += size;
    //delete the last block if the cache is full
    while (cache->c_size > MAX_CACHE_SIZE) {
        cache_delete_LRU();
    }
    pthread_rwlock_unlock(&cache->cache_lock);
}

void cache_delete_LRU(void) //delete the last block
{
    pthread_rwlock_wrlock(&cache->cache_lock);
    cache_block *temp = cache->tail->prev;
    if(temp == cache->head) {
        pthread_rwlock_unlock(&cache->cache_lock);
        return;
    }
    //pthread_rwlock_rdlock(&temp->block_lock);
    temp->prev->next = cache->tail;
    cache->tail->prev = temp->prev;
    cache->c_size -= temp->size;
    if(temp->content != NULL) {
        Free(temp->content);
    }
    //pthread_rwlock_unlock(&temp->block_lock);
    pthread_rwlock_destroy(&temp->block_lock);
    Free(temp);
    pthread_rwlock_unlock(&cache->cache_lock);
}

void print_cache(void) {
    cache_block *temp = cache->head->next;
    while (temp != cache->tail) {
        printf("! hostname: %s, path: %s, port: %d, size: %ld, freq: %d\n", temp->hostname, temp->path, temp->port, temp->size, temp->freq);
        temp = temp->next;
    }
}
