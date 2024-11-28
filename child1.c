#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

// Функция для инвертирования строки
void invert_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        const char *error_msg = "Ошибка: имя файла не передано\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file == -1) {
        const char *error_msg = "Ошибка: не удалось открыть файл для записи\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open"); 
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read - 1] = '\0';  

        invert_string(buffer);

        if (write(file, buffer, strlen(buffer)) == -1 || write(file, "\n", 1) == -1) {
            const char *error_msg = "Ошибка: не удалось записать в файл\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            close(file);
            return 1;
        }
    }

    close(file);
    return 0;
}
