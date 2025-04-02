#include "files.h"

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* mmap_file(const char *filepath, size_t *content_length) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error: Cannot open file %s: %s\n", filepath, strerror(errno));
        return NULL;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Error: Cannot stat file %s: %s\n", filepath, strerror(errno));
        close(fd);
        return NULL;
    }

    if (st.st_size <= 0) {
        fprintf(stderr, "Error: File %s is empty\n", filepath);
        close(fd);
        return NULL;
    }

    // Map the file into memory
    void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        fprintf(stderr, "Error: Cannot mmap file %s: %s\n", filepath, strerror(errno));
        close(fd);
        return NULL;
    }

    /* The file descriptor can be closed after mmap succeeds */
    close(fd);

    *content_length = st.st_size;
    return mapped;
}