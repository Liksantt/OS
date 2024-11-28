#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFER_SIZE 256

int main() {
    int pipe1[2], pipe2[2];
    char filename1[BUFFER_SIZE];
    char filename2[BUFFER_SIZE];
    ssize_t bytes_read;

    const char *prompt1 = "Введите имя файла для child1: ";
    write(STDOUT_FILENO, prompt1, strlen(prompt1));
    bytes_read = read(STDIN_FILENO, filename1, BUFFER_SIZE - 1);
    filename1[bytes_read - 1] = '\0'; 

    const char *prompt2 = "Введите имя файла для child2: ";
    write(STDOUT_FILENO, prompt2, strlen(prompt2));
    bytes_read = read(STDIN_FILENO, filename2, BUFFER_SIZE - 1);
    filename2[bytes_read - 1] = '\0'; 

    int file_check1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_check1 == -1) {
        const char *error_msg = "Ошибка: не удалось открыть файл для child1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open"); 
        return 1;
    }
    close(file_check1);  

    int file_check2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_check2 == -1) {
        const char *error_msg = "Ошибка: не удалось открыть файл для child2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");
        return 1;
    }
    close(file_check2); 

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char *error_msg = "Ошибка при создании pipe\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        const char *error_msg = "Ошибка при создании дочернего процесса 1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    } else if (pid1 == 0) {
        close(pipe1[1]); 
        dup2(pipe1[0], STDIN_FILENO);  
        execl("./child1", "./child1", filename1, NULL);  
        const char *error_msg = "Ошибка при запуске child1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        const char *error_msg = "Ошибка при создании дочернего процесса 2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    } else if (pid2 == 0) {
        close(pipe2[1]);  
        dup2(pipe2[0], STDIN_FILENO); 
        execl("./child2", "./child2", filename2, NULL);  
        const char *error_msg = "Ошибка при запуске child2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(1);
    }

    close(pipe1[0]); 
    close(pipe2[0]);  

    char input[BUFFER_SIZE];
    const char *prompt = "Введите строку: ";

    while (1) {
        write(STDOUT_FILENO, prompt, strlen(prompt));
        bytes_read = read(STDIN_FILENO, input, sizeof(input) - 1);
        if (bytes_read <= 0) {
            const char *error_msg = "Ошибка при чтении строки\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            break;
        }

        if (bytes_read > 0 && input[bytes_read - 1] == '\n') {
            input[bytes_read - 1] = '\0';
        }
        if (strlen(input) > 10) {
            write(pipe2[1], input, strlen(input) + 1);
        } else {
            write(pipe1[1], input, strlen(input) + 1);
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
