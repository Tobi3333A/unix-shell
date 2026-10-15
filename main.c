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
        int too_many_args = 0;
        argv[n] = pointer;
        while (pointer != NULL) {
            if (n >= 63) {
                too_many_args = 1;
                break;
            }
            n += 1;
            pointer = strtok(NULL, " \t");
            argv[n] = pointer;
        }

        if (too_many_args) {
            printf("too many arguments\n");
            free(line);
            continue;
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

        int pipeidx = -1;
        for (int i = 0; argv[i] != NULL; i++) {
            if (strcmp(argv[i], "|") == 0) {
                pipeidx = i;
                break;
            }
        }

        if (pipeidx == 0) {
            printf("| cannot be your first command\n");
            free(line);
            continue;
        }

        int pipefd[2];
        if (pipeidx != -1) {
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                free(line);
                continue;
            }
        }       

        pid_t pid1 = fork();
        pid_t pid2 = -1;

        if (pid1 < 0) {
            free(line);
            perror("fork failed");
            if (pipeidx != -1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }
            return 1;
        } else if (pid1 == 0) {
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
                    if (pipeidx != -1) {
                        close(pipefd[0]);
                        close(pipefd[1]);
                    }
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
                    if (pipeidx != -1) {
                        close(pipefd[0]);
                        close(pipefd[1]);
                    }
                    _exit(1);
                }
                if (dup2(slot, 1) == -1) {
                    perror("dup2 failed");
                    close(slot);
                    if (pipeidx != -1) {
                        close(pipefd[0]);
                        close(pipefd[1]);
                    }
                    _exit(1);
                }
                close(slot);
                argv[idx] = NULL;
                execvp(argv[0], argv);
                perror("execvp failed");
                if (pipeidx != -1) {
                    close(pipefd[0]);
                    close(pipefd[1]);
                }
                _exit(127);
            }

            int in_idx = -1;
            for (int i = 0; argv[i] != NULL; i++) {
                if (strcmp(argv[i], "<") == 0) {
                    in_idx = i;
                    break;
                }
            }

            if (in_idx != -1) {
                if (argv[in_idx+1] == NULL) {
                    printf("put input file\n");
                    if (pipeidx != -1) {
                        close(pipefd[0]);
                        close(pipefd[1]);
                    }
                    _exit(1);
                }
                int slot = open(argv[in_idx+1], O_RDONLY);
                if (slot == -1) {
                    perror("open failed");
                    if (pipeidx != -1) {
                        close(pipefd[0]);
                        close(pipefd[1]);
                    }
                    _exit(1);
                }
                if (dup2(slot,0) == -1) {
                    perror("dup2 failed");
                    if (pipeidx != -1) {
                        close(pipefd[0]);
                        close(pipefd[1]);
                    }
                    _exit(1);
                }
                argv[in_idx] = NULL;
                close(slot);
                
                execvp(argv[0], argv);
                perror("execvp failed");
                if (pipeidx != -1) {
                    close(pipefd[0]);
                    close(pipefd[1]);
                }
                _exit(127);
            }

            if (pipeidx != -1) {
                close(pipefd[0]);
                if (argv[pipeidx+1] == NULL) {
                    printf("second command absent\n");
                    close(pipefd[1]);
                    _exit(1);
                }
                if (dup2(pipefd[1], 1) == -1) {
                    perror("dup2 failed");
                    close(pipefd[1]);
                    _exit(1);
                }
                close(pipefd[1]);
                argv[pipeidx] = NULL;
                execvp(argv[0], argv);
                perror("execvp failed");
                _exit(127);
            }
            
            execvp(argv[0], argv);
            perror("execvp failed");
            _exit(127);
        }

        if (pipeidx != -1) {
            pid2 = fork();
            if (pid2 == -1) {
                perror("fork failed");
                close(pipefd[0]);
                close(pipefd[1]);
                free(line);
                continue;
            } else if (pid2 == 0) {
                close(pipefd[1]);
                if (dup2(pipefd[0], 0) == -1) {
                    perror("dup2 failed");
                    close(pipefd[0]);
                    _exit(1);
                }
                close(pipefd[0]);

                char **newArgv = argv + pipeidx + 1;

                execvp(newArgv[0], newArgv);
                perror("execvp failed");
                _exit(127);
            }
        }

        if (pid1 > 0) {
            if (pipeidx != -1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }
            if (waitpid(pid1, NULL, 0) == -1) {
                perror("waitpid failed");
                free(line);
                continue;
            }
            if (pid2 > 0) {
                if (waitpid(pid2, NULL, 0) == -1) {
                    perror("waitpid failed");
                    free(line);
                    continue;
                }
            }
        }

        free(line);
    }

    return 0;
}