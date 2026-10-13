#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int status = 0;
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
                status = chdir(argv[1]);
                if (status == -1) {
                    perror("cd");
                }
            } else {
                char *home = getenv("HOME");
                if (home == NULL) {
                    perror("cd");
                } else {
                    status = chdir(home);
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
            int idx = -1;
            for (int i = 0; argv[i] != NULL; i++) {
                if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0) {
                    idx = i;
                    break;
                }
            }
            if (idx != -1) {
                if (argv[idx+1] == NULL) {
                    printf("file directory absent\n");
                    _exit(1);
                }
                int slot = -1;
                if (strcmp(argv[idx], ">") == 0) {
                    slot = open(argv[idx+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                } else {
                    slot = open(argv[idx+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                }
                if (slot == -1) {
                    perror("open failed");
                    _exit(1);
                }
                if (dup2(slot, 1) == -1) {
                    perror("dup2 failed");
                    close(slot);
                    _exit(1);
                }
                close(slot);
                argv[idx] = NULL;
                execvp(argv[0], argv);
                perror("execvp failed");
                _exit(127);
            }
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