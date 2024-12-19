#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_SIZE 256

bool is_vowel(char c) {
    c = (c >= 'A' && c <= 'Z') ? c + 32 : c;
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

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
        const char *error_msg = "Error: The file name was not passed\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        return 1;
    }
    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file == -1) {
        const char *error_msg = "Error: the file could not be opened for writing\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        perror("open");
        return 1;
    }
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read - 1] = '\0';
        if(strcmp(buffer, "exit") == 0){
            close(file);
            return 0;
        }
        invert_string(buffer);
        if (write(file, buffer, strlen(buffer)) == -1 || write(file, "\n", 1) == -1) {
            const char *error_msg = "Error: failed to write to a file\n";
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            close(file);
            return 1;
        }
    }
    close(file);
    return 0;
}