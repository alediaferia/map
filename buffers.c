#include "buffers.h"

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define FALLBACK_BUFFER_SIZE 4069

void bufflush(const char *buf, size_t len, FILE *dst) {
    if (fwrite(buf, sizeof(char), len, dst) < len) {
        if (ferror(dst) > 0) {
            fprintf(stderr, "Error: bufflush: unable to flush buffer: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Error: bufflush: unable to flush buffer\n");
        }
        exit(EXIT_FAILURE);
    }
}

size_t calc_iobufsize(void) {
    struct stat s;

    int fd = fcntl(STDIN_FILENO, F_DUPFD, 0);
    fstat(fd, &s);
    close(fd);

    size_t bufsize = s.st_blksize;
    if (bufsize <= 0) {
        /* try to use a page size from the system */
        bufsize = sysconf(_SC_PAGESIZE);
        if (bufsize <= 0) {
            bufsize = FALLBACK_BUFFER_SIZE; // arbitrary fall back
        }
    }

    return bufsize;
}
