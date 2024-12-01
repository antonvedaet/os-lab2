#include <iostream>
#include <filesystem>
#include <string>
#include <dlfcn.h>
#include <vector>
#include <cstring>
#include "lab2_cache.h"
namespace fs = std::filesystem;

void find_file(const std::string &filename, const fs::path &search_path)
{
    bool found = false;

    for (const auto &entry : fs::recursive_directory_iterator(search_path))
    {
        if (entry.is_regular_file() && entry.path().filename() == filename)
        {
            int fd = lab2_open(entry.path().c_str(), 4);
            if (fd < 0)
            {
                std::cerr << "Ошибка открытия файла: " << entry.path() << std::endl;
                continue;
            }

            char buffer[512];
            ssize_t bytes_read = lab2_read(fd, buffer, sizeof(buffer));
            if (bytes_read > 0)
            {
                std::cout << "Файл найден: " << entry.path() << std::endl;
                found = true;
            }
            else
            {
                std::cerr << "Ошибка чтения файла: " << entry.path() << std::endl;
            }
            lab2_close(fd);
        }
    }

    if (!found)
    {
        std::cout << "file not found" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "usage: " << argv[0] << " <filename> <search_path> <n>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    fs::path search_path = argv[2];
    int repeat = std::stoi(argv[3]);

    for (int i = 0; i < repeat; ++i)
    {
        find_file(filename, search_path);
    }

    return 0;
}
