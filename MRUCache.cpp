#include <fcntl.h>  
#include <unistd.h>     
#include <sys/types.h>  
#include <stdlib.h>  
#include <string.h> 
#include <errno.h>   
#include <stdio.h>      

#define BLOCK_SIZE 4096      
#define CACHE_CAPACITY 128   
typedef struct CacheBlock {
    int fd;               
    off_t offset;         
    char data[BLOCK_SIZE];
    int is_dirty;         
} CacheBlock;


static CacheBlock cache[CACHE_CAPACITY];
static int mru_index[CACHE_CAPACITY];  


static void cache_init() {
    memset(cache, 0, sizeof(cache));
    for (int i = 0; i < CACHE_CAPACITY; i++) {
        mru_index[i] = i;  
        cache[i].fd = -1; 
    }
}


static int cache_find(int fd, off_t offset) {
    for (int i = 0; i < CACHE_CAPACITY; i++) {
        if (cache[mru_index[i]].fd == fd && cache[mru_index[i]].offset == offset) {
            return i;
        }
    }
    return -1;
}


static int cache_add(int fd, off_t offset, const void *data, size_t size, int is_dirty) {
    int evict_index = mru_index[CACHE_CAPACITY - 1]; 
    if (cache[evict_index].is_dirty) {
        pwrite(cache[evict_index].fd, cache[evict_index].data, BLOCK_SIZE, cache[evict_index].offset);
    }

    cache[evict_index].fd = fd;
    cache[evict_index].offset = offset;
    memcpy(cache[evict_index].data, data, size);
    cache[evict_index].is_dirty = is_dirty;

    for (int i = CACHE_CAPACITY - 1; i > 0; i--) {
        mru_index[i] = mru_index[i - 1];
    }
    mru_index[0] = evict_index;

    return 0;
}

static void cache_update_mru(int cache_idx) {
    int block_index = mru_index[cache_idx];
    for (int i = cache_idx; i > 0; i--) {
        mru_index[i] = mru_index[i - 1];
    }
    mru_index[0] = block_index;
}

ssize_t lab2_read(int fd, void *buf, size_t count) {
    off_t offset = lseek(fd, 0, SEEK_CUR);
    int block_index = cache_find(fd, offset);

    if (block_index >= 0) {
        memcpy(buf, cache[mru_index[block_index]].data, count);
        cache_update_mru(block_index);
    } else {
        ssize_t bytes_read = pread(fd, buf, count, offset);
        if (bytes_read < 0) return -1;
        cache_add(fd, offset, buf, bytes_read, 0);
    }

    lseek(fd, offset + count, SEEK_SET);
    return count;
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
    off_t offset = lseek(fd, 0, SEEK_CUR);
    int block_index = cache_find(fd, offset);

    if (block_index >= 0) {
        memcpy(cache[mru_index[block_index]].data, buf, count);
        cache[mru_index[block_index]].is_dirty = 1;
        cache_update_mru(block_index);
    } else {
        cache_add(fd, offset, buf, count, 1);
    }

    lseek(fd, offset + count, SEEK_SET);
    return count;
}

int lab2_fsync(int fd) {
    for (int i = 0; i < CACHE_CAPACITY; i++) {
        if (cache[i].fd == fd && cache[i].is_dirty) {
            pwrite(fd, cache[i].data, BLOCK_SIZE, cache[i].offset);
            cache[i].is_dirty = 0;
        }
    }
    return 0;
}

int lab2_open(const char *path) {
    int fd = open(path, O_RDWR | O_DIRECT);
    if (fd < 0) {
        perror("Ошибка открытия файла");
        return -1;
    }
    return fd;
}

int lab2_close(int fd) {
    lab2_fsync(fd);
    return close(fd);
}

off_t lab2_lseek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}
