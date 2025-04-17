#include "cmd.h"

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FILE* runcmd(int argc, char *argv[]) {
    if (argc == 0) {
        return NULL;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) { // Child process
        close(pipefd[0]);  // Close read end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(argv[0], argv);
        // If execvp returns, there was an error
        fprintf(stderr, "Error executing command: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[1]);  // Close write end
    FILE *fp = fdopen(pipefd[0], "r");
    if (fp == NULL) {
        fprintf(stderr, "Error creating file stream: %s\n", strerror(errno));
        close(pipefd[0]);
        return NULL;
    }

    return fp;
}
