#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <vector>
#include <deque>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <vector>

struct CachePage {
    off_t offset;
    std::vector<char> data;
    bool dirty;
    bool used;
};

struct CachedFile {
    int fd;
    std::map<off_t, CachePage> cache;
    std::deque<off_t> cache_queue;
    size_t max_cache_size;
    off_t current_offset;
};

std::map<int, CachedFile> open_files;

const size_t PAGE_SIZE = 4096;

inline off_t page_offset(off_t offset) {
    return offset & ~(PAGE_SIZE - 1);
}

void evict_page(CachedFile &file) {
    while (!file.cache_queue.empty()) {
        off_t offset = file.cache_queue.back();
        file.cache_queue.pop_back();

        CachePage &page = file.cache[offset];
        if (page.used) {
            page.used = false;
            file.cache_queue.push_front(offset);
        } else {
            if (page.dirty) {
                std::cout << "Evicting dirty page at offset: " << offset << std::endl;

                size_t actual_size_to_write = 0;
                for (size_t i = 0; i < PAGE_SIZE; ++i) {
                    if (page.data[i] != 0) {
                        actual_size_to_write = i + 1;
                    }
                }

                lseek(file.fd, offset, SEEK_SET);
                write(file.fd, page.data.data(), actual_size_to_write);
            } else {
                std::cout << "Evicting clean page at offset: " << offset << std::endl;
            }
            file.cache.erase(offset);
            return;
        }
    }
}

int lab2_open(const char *path, size_t max_cache_size = 4) {
    int fd = open(path, O_RDWR);
    if (fd == -1) return -1;

    CachedFile cachedFile = {fd, {}, {}, max_cache_size, 0};
    open_files[fd] = cachedFile;
    return fd;
}

int lab2_close(int fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    for (auto &entry : it->second.cache) {
        if (entry.second.dirty) {
            lseek(fd, entry.first, SEEK_SET);
            write(fd, entry.second.data.data(), entry.second.data.size());
        }
    }
    close(fd);
    open_files.erase(fd);
    return 0;
}

ssize_t lab2_read(int fd, void *buf, size_t count) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    CachedFile &file = it->second;
    size_t bytesRead = 0;

    while (count > 0) {
        off_t page_offset = file.current_offset & ~(PAGE_SIZE - 1);
        size_t offset_in_page = file.current_offset % PAGE_SIZE;
        size_t to_read = std::min(count, PAGE_SIZE - offset_in_page);

        if (file.cache.find(page_offset) == file.cache.end()) {
            if (file.cache.size() >= file.max_cache_size) evict_page(file);

            CachePage page;
            page.offset = page_offset;
            page.data.resize(PAGE_SIZE, 0);
            page.dirty = false;
            page.used = true;

            lseek(fd, page_offset, SEEK_SET);
            ssize_t result = read(fd, page.data.data(), PAGE_SIZE);
            if (result < 0) {
                std::cerr << "Error reading from file at offset " << page_offset << std::endl;
                return -1;
            }
            file.cache[page_offset] = page;
            file.cache_queue.push_front(page_offset);
            std::cout << "Read page at offset: " << page_offset << ", bytes read: " << result << std::endl;
        } else {
            file.cache_queue.erase(std::find(file.cache_queue.begin(), file.cache_queue.end(), page_offset));
            file.cache_queue.push_front(page_offset);
        }

        CachePage &page = file.cache[page_offset];
        std::memcpy((char *)buf + bytesRead, page.data.data() + offset_in_page, to_read);
        page.used = true;

        file.current_offset += to_read;
        bytesRead += to_read;
        count -= to_read;
    }

    return bytesRead;
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    CachedFile &file = it->second;
    size_t bytesWritten = 0;

    while (count > 0) {
        off_t page_offset = file.current_offset & ~(PAGE_SIZE - 1);
        size_t offset_in_page = file.current_offset % PAGE_SIZE;
        size_t to_write = std::min(count, PAGE_SIZE - offset_in_page);

        if (file.cache.find(page_offset) == file.cache.end()) {
            if (file.cache.size() >= file.max_cache_size) evict_page(file);

            CachePage page;
            page.offset = page_offset;
            page.data.resize(PAGE_SIZE, 0);
            page.dirty = true;
            page.used = true;

            file.cache[page_offset] = page;
            file.cache_queue.push_front(page_offset);
        } else {
            file.cache_queue.erase(std::find(file.cache_queue.begin(), file.cache_queue.end(), page_offset));
            file.cache_queue.push_front(page_offset);
        }

        CachePage &page = file.cache[page_offset];
        std::memcpy(page.data.data() + offset_in_page, (const char *)buf + bytesWritten, to_write);
        page.dirty = true;
        page.used = true;

        file.current_offset += to_write;
        bytesWritten += to_write;
        count -= to_write;
    }

    return bytesWritten;
}

off_t lab2_lseek(int fd, off_t offset, int whence) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    CachedFile &file = it->second;
    switch (whence) {
        case SEEK_SET:
            file.current_offset = offset;
            break;
        case SEEK_CUR:
            file.current_offset += offset;
            break;
        case SEEK_END:
            struct stat st;
            if (fstat(fd, &st) == -1) return -1;
            file.current_offset = st.st_size + offset;
            break;
        default:
            return -1;
    }

    return file.current_offset;
}

int lab2_fsync(int fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    for (auto &entry : it->second.cache) {
        if (entry.second.dirty) {
            std::cout << "Syncing dirty page at offset: " << entry.first << std::endl;

            size_t actual_size_to_write = 0;
            for (size_t i = 0; i < PAGE_SIZE; ++i) {
                if (entry.second.data[i] != 0) {
                    actual_size_to_write = i + 1;
                }
            }

            lseek(fd, entry.first, SEEK_SET);
            write(fd, entry.second.data.data(), actual_size_to_write);
            entry.second.dirty = false;
        }
    }

    return fsync(fd);
}
