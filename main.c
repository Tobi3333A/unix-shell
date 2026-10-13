#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main() {
    while (1) {
        printf("klawshell> ");
        fflush(stdout);

        char *line = NULL;
        size_t capacity = 0;

        ssize_t length = getline(&line, &capacity, stdin);

        if (length == -1) {
            if (feof(stdin)) {
                free(line);
                return 0;
            }
            free(line);
            return 1;
        }

        if (line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            free(line);
            break;
        }

        printf("You typed: %s\n", line);
        
        free(line);
    }

    return 0;
}