#ifndef LAB2_CACHE_H
#define LAB2_CACHE_H

#include <sys/types.h>  
#include <stddef.h>  

#define BLOCK_SIZE 4096      
#define CACHE_CAPACITY 128   


typedef struct CacheBlock {
    int fd;              
    off_t offset;        
    char data[BLOCK_SIZE];
    int is_dirty;         
} CacheBlock;


void cache_init(void);


int cache_find(int fd, off_t offset);


int cache_add(int fd, off_t offset, const void *data, size_t size, int is_dirty);


void cache_update_mru(int cache_idx);


ssize_t lab2_read(int fd, void *buf, size_t count);


ssize_t lab2_write(int fd, const void *buf, size_t count);


int lab2_fsync(int fd);


int lab2_open(const char *path);


int lab2_close(int fd);


off_t lab2_lseek(int fd, off_t offset, int whence);

#endif 
