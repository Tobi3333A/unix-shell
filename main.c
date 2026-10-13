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

        if (strcmp(argv[0], "exit") == 0) {
            free(line);
            break;
        } else if (strcmp(argv[0], "cd") == 0) {
            if (argv[1] != NULL) {
                int status = chdir(argv[1]);
                if (status == -1) {
                    perror("cd");
                }
            } else {
                char *home = getenv("HOME");
                if (home == NULL) {
                    perror("cd");
                } else {
                    int status = chdir(home);
                    if (status == -1) {
                        perror("cd");
                    }
                }
            }
            free(line);
            continue;
        } else if (strcmp(argv[0], "help") == 0) {
            printf("cd\npwd\nls\nexit\n");
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
            _exit(127);                          // Why is this better than just exit(1);
        } else {
            waitpid(pid, NULL, 0);
        }

        free(line);
    }

    return 0;
}