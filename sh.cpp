#include <iostream>
#include <cstring>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <linux/sched.h>
#include <sched.h>
#include <cstdlib>
#include <vector>

int child_func(char *cmd[])
{
    execvp(cmd[0], cmd);
    std::cerr << "error during command execution: " << cmd[0] << std::endl;
    return 1;
}

void execute_command(char *cmd[])
{
    auto start = std::chrono::high_resolution_clock::now();

    struct clone_args
    {
        __aligned_u64 flags;
        __aligned_u64 pidfd;
        __aligned_u64 child_tid;
        __aligned_u64 parent_tid;
        __aligned_u64 exit_signal;
        __aligned_u64 stack;
        __aligned_u64 stack_size;
        __aligned_u64 tls;
        __aligned_u64 tls_size;
        __aligned_u64 set_tid;
        __aligned_u64 close_fd;
    };

    struct clone_args args = {
        .flags = CLONE_FS | CLONE_FILES,
        .exit_signal = SIGCHLD};

    pid_t pid = syscall(SYS_clone3, &args, sizeof(struct clone_args));
    if (pid == -1)
    {
        std::cerr << "error during process creation: " << strerror(errno) << std::endl;
        return;
    }
    else if (pid == 0)
    {
        int result = child_func(cmd);
        free(reinterpret_cast<void *>(args.stack));
        _exit(result);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "execution time: " << elapsed.count() << " seconds\n";

        free(reinterpret_cast<void *>(args.stack));
    }
}

// копия прошлой функции просто выполняет много команд сразу
void start_multiple_instances(int n, char *cmd[])
{
    std::vector<pid_t> pids;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; ++i)
    {
        struct clone_args
        {
            __aligned_u64 flags;
            __aligned_u64 pidfd;
            __aligned_u64 child_tid;
            __aligned_u64 parent_tid;
            __aligned_u64 exit_signal;
            __aligned_u64 stack;
            __aligned_u64 stack_size;
            __aligned_u64 tls;
            __aligned_u64 tls_size;
            __aligned_u64 set_tid;
            __aligned_u64 close_fd;
        };

        struct clone_args args = {
            .flags = CLONE_FS | CLONE_FILES,
            .exit_signal = SIGCHLD};

        pid_t pid = syscall(SYS_clone3, &args, sizeof(struct clone_args));
        if (pid == -1)
        {
            std::cerr << "error during process creation: " << strerror(errno) << std::endl;
            return;
        }
        else if (pid == 0)
        {
            int result = child_func(cmd);
            free(reinterpret_cast<void *>(args.stack));
            _exit(result);
        }
        else
        {
            pids.push_back(pid);
        }
    }

    for (pid_t pid : pids)
    {
        int status;
        waitpid(pid, &status, 0);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "execution time: " << elapsed.count() << " seconds\n";
}

int main()
{
    while (true)
    {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit")
        {
            break;
        }

        char *cmd[100];
        char *input_copy = strdup(input.c_str());
        char *token = strtok(input_copy, " ");
        int i = 0;

        while (token != nullptr)
        {
            cmd[i++] = token;
            token = std::strtok(nullptr, " ");
        }
        cmd[i] = nullptr;

        if (i > 1 && std::string(cmd[0]) == "start")
        {
            int n = std::stoi(cmd[1]);
            char *new_cmd[100];
            for (int j = 2; j < i; ++j)
            {
                new_cmd[j - 2] = cmd[j];
            }
            new_cmd[i - 2] = nullptr;
            start_multiple_instances(n, new_cmd);
        }
        else
        {
            execute_command(cmd);
        }

        free(input_copy);
    }
    return 0;
}