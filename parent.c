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

    // Запрашиваем у пользователя имена файлов
    const char *prompt1 = "Введите имя файла для child1: ";
    write(STDOUT_FILENO, prompt1, strlen(prompt1));
    bytes_read = read(STDIN_FILENO, filename1, BUFFER_SIZE - 1);
    filename1[bytes_read - 1] = '\0';  // Удаляем символ новой строки

    const char *prompt2 = "Введите имя файла для child2: ";
    write(STDOUT_FILENO, prompt2, strlen(prompt2));
    bytes_read = read(STDIN_FILENO, filename2, BUFFER_SIZE - 1);
    filename2[bytes_read - 1] = '\0';  // Удаляем символ новой строки

    // Проверяем возможность создания файлов для записи
    int file_check1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_check1 == -1) {
        const char *error_msg = "Ошибка: не удалось открыть файл для child1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");  // Выводим детальное сообщение об ошибке
        return 1;
    }
    close(file_check1);  // Закрываем файл, поскольку он будет использоваться только дочерним процессом

    int file_check2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_check2 == -1) {
        const char *error_msg = "Ошибка: не удалось открыть файл для child2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");  // Выводим детальное сообщение об ошибке
        return 1;
    }
    close(file_check2);  // Закрываем файл

    // Создаем два канала
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char *error_msg = "Ошибка при создании pipe\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    // Создаем первый дочерний процесс для pipe1 (child1)
    pid_t pid1 = fork();
    if (pid1 == -1) {
        const char *error_msg = "Ошибка при создании дочернего процесса 1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    } else if (pid1 == 0) {
        // Дочерний процесс child1
        close(pipe1[1]);  // Закрываем конец записи pipe1
        dup2(pipe1[0], STDIN_FILENO);  // Перенаправляем стандартный ввод на pipe1
        execl("./child1", "./child1", filename1, NULL);  // Запускаем child1 с именем файла
        const char *error_msg = "Ошибка при запуске child1\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(1);
    }

    // Создаем второй дочерний процесс для pipe2 (child2)
    pid_t pid2 = fork();
    if (pid2 == -1) {
        const char *error_msg = "Ошибка при создании дочернего процесса 2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    } else if (pid2 == 0) {
        // Дочерний процесс child2
        close(pipe2[1]);  // Закрываем конец записи pipe2
        dup2(pipe2[0], STDIN_FILENO);  // Перенаправляем стандартный ввод на pipe2
        execl("./child2", "./child2", filename2, NULL);  // Запускаем child2 с именем файла
        const char *error_msg = "Ошибка при запуске child2\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(1);
    }

    // Родительский процесс
    close(pipe1[0]);  // Закрываем конец чтения pipe1
    close(pipe2[0]);  // Закрываем конец чтения pipe2

    char input[BUFFER_SIZE];
    const char *prompt = "Введите строку: ";

    // Ввод строк пользователем
    while (1) {
        write(STDOUT_FILENO, prompt, strlen(prompt));
        bytes_read = read(STDIN_FILENO, input, sizeof(input) - 1);
        if (bytes_read <= 0) {
            const char *error_msg = "Ошибка при чтении строки\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            break;
        }

        // Удаляем символ новой строки, если он есть
        if (bytes_read > 0 && input[bytes_read - 1] == '\n') {
            input[bytes_read - 1] = '\0';
        }

        // Отправка строки в соответствующий pipe
        if (strlen(input) > 10) {
            write(pipe2[1], input, strlen(input) + 1);
        } else {
            write(pipe1[1], input, strlen(input) + 1);
        }
    }

    // Закрываем каналы записи
    close(pipe1[1]);
    close(pipe2[1]);

    // Ожидаем завершения дочерних процессов
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
