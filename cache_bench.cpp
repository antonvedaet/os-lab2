#include <iostream>
#include <fstream>
#include <chrono>
#include "lab2_cache.h"

#define FILE_PATH "benchmark_file_cache.txt"
#define FILE_SIZE (1024 * 1024 * 100) // 10 MB
#define BLOCK_SIZE 4096

void generate_file(const char *path, size_t size)
{
    std::ofstream file(path, std::ios::binary);
    char buffer[BLOCK_SIZE] = {0};
    for (size_t i = 0; i < size / BLOCK_SIZE; ++i)
    {
        file.write(buffer, BLOCK_SIZE);
    }
    file.close();
}

int main()
{

    generate_file(FILE_PATH, FILE_SIZE);

    int fd = lab2_open(FILE_PATH, 5);
    if (fd < 0)
    {
        std::cerr << "Failed to open file" << std::endl;
        return -1;
    }

    char buffer[BLOCK_SIZE];
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; ++i)
    {
        lab2_read(fd, buffer, BLOCK_SIZE);
        lab2_write(fd, buffer, BLOCK_SIZE);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    lab2_close(fd);

    std::cout << "Time taken with cache: " << duration.count() << " seconds" << std::endl;
    if (std::remove(FILE_PATH) != 0)
    {
        std::cerr << "Error deleting file" << std::endl;
    }
    else
    {
        std::cout << "File successfully deleted" << std::endl;
    }

    return 0;
}
