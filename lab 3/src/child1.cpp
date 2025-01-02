#include <iostream>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <semaphore.h>

const size_t SHARED_MEMORY_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Not enough arguments" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    const char* shm_name = argv[2];
    const char* sem_parent_name = argv[3];
    const char* sem_child_name = argv[4];

    sem_t* s_parent = sem_open(sem_parent_name, 0);
    sem_t* s_child1 = sem_open(sem_child_name, 0);

    if (s_parent == SEM_FAILED || s_child1 == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    void* shared_memory = mmap(nullptr, SHARED_MEMORY_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }

    while (true) {
        sem_wait(s_child1);

        std::string data(static_cast<char*>(shared_memory));
        if (data.empty()) {
            break;  
        }

        std::cout << "Child1: Received data: " << data << std::endl;

        std::reverse(data.begin(), data.end());
        file << data << std::endl;

        sem_post(s_parent); 
        std::cout << "Child1: Processed data: " << data << std::endl;
    }

    file.close();
    munmap(shared_memory, SHARED_MEMORY_SIZE);

    sem_close(s_parent);
    sem_close(s_child1);

    return 0;
}
