#include <iostream>
#include <fstream>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define FILE_PATH "benchmark_file.txt"
#define FILE_SIZE (1024 * 1024 * 100) 
#define BLOCK_SIZE 4096

void generate_file(const char* path, size_t size) {
    std::ofstream file(path, std::ios::binary);
    char buffer[BLOCK_SIZE] = {0};
    for (size_t i = 0; i < size / BLOCK_SIZE; ++i) {
        file.write(buffer, BLOCK_SIZE);
    }
    file.close();
}

int main() {
    generate_file(FILE_PATH, FILE_SIZE);

    int fd = open(FILE_PATH, O_RDWR| O_DIRECT);
    if (fd < 0) {
        std::cerr << "Failed to open file" << std::endl;
        return -1;
    }

    char buffer[BLOCK_SIZE];
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; ++i) {
        read(fd, buffer, BLOCK_SIZE);
        lseek(fd, -BLOCK_SIZE, SEEK_CUR);
        write(fd, buffer, BLOCK_SIZE);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    close(fd);

    std::cout << "Time taken without cache: " << duration.count() << " seconds" << std::endl;

    return 0;
}
