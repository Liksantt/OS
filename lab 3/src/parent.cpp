#include "parent.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <semaphore.h>

sem_t *s_parent, *s_child1, *s_child2;
const size_t SHARED_MEMORY_SIZE = 1024;
const char* SHM_NAME1 = "/shm_child1";
const char* SHM_NAME2 = "/shm_child2";
const char* SEM_PARENT_NAME = "/s_parent";
const char* SEM_CHILD1_NAME = "/s_child1";
const char* SEM_CHILD2_NAME = "/s_child2";

int main() {
    return Parent(std::cin);
}

int Parent(std::istream& in) {
    std::string filename1, filename2;

    // Попросим пользователя ввести имена файлов
    std::cout << "Enter the first file name: ";
    std::getline(in, filename1);
    std::cout << "Enter the second file name: ";
    std::getline(in, filename2);

    // Проверим, что имена файлов не пустые
    if (filename1.empty() || filename2.empty()) {
        std::cerr << "Both file names must be provided!" << std::endl;
        return 1;
    }

    // Создаем семафоры
    s_parent = sem_open(SEM_PARENT_NAME, O_CREAT, 0666, 0);
    s_child1 = sem_open(SEM_CHILD1_NAME, O_CREAT, 0666, 0);
    s_child2 = sem_open(SEM_CHILD2_NAME, O_CREAT, 0666, 0);
    if (s_parent == SEM_FAILED || s_child1 == SEM_FAILED || s_child2 == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    int shm_fd1 = shm_open(SHM_NAME1, O_CREAT | O_RDWR, 0666);
    int shm_fd2 = shm_open(SHM_NAME2, O_CREAT | O_RDWR, 0666);
    if (shm_fd1 == -1 || shm_fd2 == -1) {
        perror("shm_open");
        return 1;
    }

    ftruncate(shm_fd1, SHARED_MEMORY_SIZE);
    ftruncate(shm_fd2, SHARED_MEMORY_SIZE);

    void* shared_memory1 = mmap(nullptr, SHARED_MEMORY_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd1, 0);
    void* shared_memory2 = mmap(nullptr, SHARED_MEMORY_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd2, 0);

    if (shared_memory1 == MAP_FAILED || shared_memory2 == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        return 1;
    }

    if (pid1 == 0) {
        execl("./child1", "child1", filename1.c_str(), SHM_NAME1, SEM_PARENT_NAME, SEM_CHILD1_NAME, nullptr);
        perror("execl");
        return 1;
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        return 1;
    }

    if (pid2 == 0) {
        execl("./child2", "child2", filename2.c_str(), SHM_NAME2, SEM_PARENT_NAME, SEM_CHILD2_NAME, nullptr);
        perror("execl");
        return 1;
    }

    memset(shared_memory1, 0, SHARED_MEMORY_SIZE);
    memset(shared_memory2, 0, SHARED_MEMORY_SIZE);

    std::string input;
    while (true) {
        std::getline(in, input);
        if (input == "quit") break;

        if (input.length() > 10) {
            // Отправляем данные в shared_memory2 и сигнализируем child2
            strncpy(static_cast<char*>(shared_memory2), input.c_str(), SHARED_MEMORY_SIZE - 1);
            sem_post(s_child2); // Пробуждаем child2
            std::cout << "Parent: Sent data to child2: " << input << std::endl;

            sem_wait(s_parent); // Родитель засыпает, ждет сигнала от child2
            std::cout << "Parent: Woke up after child2 processed data." << std::endl;
        } else {
            // Отправляем данные в shared_memory1 и сигнализируем child1
            strncpy(static_cast<char*>(shared_memory1), input.c_str(), SHARED_MEMORY_SIZE - 1);
            sem_post(s_child1); // Пробуждаем child1
            std::cout << "Parent: Sent data to child1: " << input << std::endl;

            sem_wait(s_parent); 
            std::cout << "Parent: Woke up after child1 processed data." << std::endl;
        }
    }

    strncpy(static_cast<char*>(shared_memory1), "", SHARED_MEMORY_SIZE - 1);
    strncpy(static_cast<char*>(shared_memory2), "", SHARED_MEMORY_SIZE - 1);
    sem_post(s_child1); 
    sem_post(s_child2); 

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    // Освобождаем ресурсы
    munmap(shared_memory1, SHARED_MEMORY_SIZE);
    munmap(shared_memory2, SHARED_MEMORY_SIZE);
    shm_unlink(SHM_NAME1);
    shm_unlink(SHM_NAME2);

    sem_close(s_parent);
    sem_close(s_child1);
    sem_close(s_child2);
    sem_unlink(SEM_PARENT_NAME);
    sem_unlink(SEM_CHILD1_NAME);
    sem_unlink(SEM_CHILD2_NAME);

    return 0;
}
