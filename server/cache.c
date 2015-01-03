/*
    This file is part of memoryLeak.

    memoryLeak is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    memoryLeak is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with memoryLeak; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "cache.h"
#include <pthread.h>
#include <assert.h>
#include "structs.h"
#include "list.h"


static unsigned long long int max_mem;
static unsigned long long int mem_size;
pthread_rwlock_t cache_lock;
static struct _list_el *cache;

struct _cache_el *
cache_find(char filename[1024]) 
{
    // WARNING: cache should be locked for reading at this moment
    if (cache == NULL) // if cache is empty
        return NULL;
    // create local copy of cache pointer and remeber start point
    struct _list_el *local = cache, *start = cache;
    do {
        // check if cache hit
        if (strcmp(filename, ((struct _cache_el*) local->el)->filename) == 0)
            return (struct _cache_el*) local->el;
        local = local->next;
    } while (local != start); // checking for cycle

    return NULL; // cache miss
}


void *
cache_load_memory(char filename[1024], struct stat *file_stat)
{
    int fd = open(filename, O_RDONLY);
    void *addr;

    addr = mmap(NULL, file_stat->st_size, PROT_READ, MAP_SHARED,
                fd, 0);
    if (addr == MAP_FAILED) {
        return NULL;
    } else {
        return addr;
    }
}

void
cache_read_write(int fd, char filename[1024], unsigned long long int size)
{
    char buffer[1024];
    int ffd = open(filename, O_RDONLY);
    int n;
    while (size > 0) {
        n = read(ffd, buffer, 1024);
        write(fd, buffer, n);
        size -= n;
    }
}

void cache_free_memory()
{
    printf("[!] Need to alocate %db memory\n", mem_size - max_mem);
    while(mem_size > max_mem) {
        printf("[?] Cache points to: %p, mem: %d, max: %d\n", cache, mem_size, max_mem);
        while (((struct _cache_el*) cache->el)->usage != 0) {
            ((struct _cache_el*) cache->el)->usage -= 1;
            cache = cache->next;
        }
        struct _cache_el *el = (struct _cache_el*) cache->el;
        munmap(el->addr, el->size);
        printf("[!] Freed %d bytes by removing %s\n", el->size, el->filename);
        mem_size -= el->size;
        free(el);
        list_delete(&cache);
    }
}

void
cache_retrieve_file(int fd, char filename[1024], struct stat *file_stat)
{
    if (file_stat->st_size > max_mem) {
        // file is too big, need buffered read
        printf("[!] File too big\n");
        cache_read_write(fd, filename, file_stat->st_size);
    } else {
        // checking cache
        printf("[!] Checking cache\n");
        pthread_rwlock_rdlock(&cache_lock); // lock cache for reading
        struct _cache_el *e = cache_find(filename);
        if (e == NULL) {
            // no file in cache
            printf("[!] Cache miss\n");
            pthread_rwlock_unlock(&cache_lock);
            pthread_rwlock_wrlock(&cache_lock);
            // double check if file is still missing
            e = cache_find(filename);
            if (e == NULL) { // still nope
                mem_size += file_stat->st_size;
                if (mem_size > max_mem) { 
                    // need to free memory
                    cache_free_memory();
                }
                // create new cache element
                e = malloc(sizeof(struct _cache_el));
                e->addr = cache_load_memory(filename, file_stat);
                e->size = file_stat->st_size;
                strncpy(e->filename, filename, 1024);
                e->mtime = file_stat->st_mtim.tv_sec;
                e->usage = 1;
                // insert to cache
                list_insert(&cache, e);
                printf("[+] Added file to cache\n");
            }
            write(fd, e->addr, file_stat->st_size);
            pthread_rwlock_unlock(&cache_lock);
            return;
        }
        if (e->mtime < file_stat->st_mtim.tv_sec) {
            // refresh file
            // TODO
        }
        printf("[!] Cache hit!\n");
        write(fd, e->addr, file_stat->st_size);
        pthread_rwlock_unlock(&cache_lock);
        //void *file = cache_from_memory(filename, file_stat);
    }
}

void
cache_get_request(int fd, char filename[1024])
{
    struct stat file_stat;
    if(filename[0] != '.' && !stat(filename, &file_stat)) {
        char answer[1024];
        sprintf(answer, "OK %d\n", (int) file_stat.st_size);
        write(fd, answer, strlen(answer));
        cache_retrieve_file(fd, filename, &file_stat);
    } else {
        write(fd, "NOTFOUND\n", 9);
    }
}

void 
cache_handle_request(int fd, char buffer[1024])
{
    char cmd[16];
    sscanf(buffer, "%15s", cmd);
    if (strcmp(cmd, "GET") == 0) {
        char filename[1024];
        sscanf(buffer + 4, "%1023s", filename);
        printf("[!] Requested %s\n", filename);
        cache_get_request(fd, filename);
    }
}

void
set_cache_size(unsigned long long int size)
{
    mem_size = 0;
    max_mem = size;
    pthread_rwlock_init(&cache_lock, NULL);
    cache = NULL;
}

void
cache_delete_all()
{
    mem_size += max_mem;
    cache_free_memory();
    pthread_rwlock_destroy(&cache_lock);
    assert(cache == NULL);
}