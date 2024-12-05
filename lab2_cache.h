#ifndef CACHE_BLOCK_H
#define CACHE_BLOCK_H

#include <cstddef>
#include <cstdint>

int lab2_open(const char *path, size_t max_cache_size);
int lab2_close(int fd);
ssize_t lab2_read(int fd, void *buf, size_t count);
ssize_t lab2_write(int fd, const void *buf, size_t count);
off_t lab2_lseek(int fd, off_t offset, int whence);
int lab2_fsync(int fd);
void output_cache_stats(int fd);
#endif