#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

        char *argv[64];
        char *pointer = strtok(line, " \t");
        int n= 0;
        argv[n] = pointer;
        while (pointer != NULL) {
            n += 1;
            pointer = strtok(NULL, " \t");
            argv[n] = pointer;
        }

        if (argv[0] == NULL) {
            free(line);
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            free(line);
            perror("fork failed");
            return 1;
        } else if (pid == 0) {
            execvp(argv[0], argv);
            perror("execvp failed");
            __exit(127);                          // Why is this better than just exit(1);
        } else {
            waitpid(pid, NULL, 0);
        }
        

        free(line);
    }

    return 0;
}