/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: buffers.c
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "buffers.h"

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

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

char* bufalloc(size_t size) {
    char *buffer = calloc(size, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Unable to allocate buffer. Check that your system has enough memory (%ld bytes)", size);
        exit(EXIT_FAILURE);
    }

    return buffer;
}

void bufencap(const char *buffer, size_t size, size_t *pos, FILE *dst) {
    if ((*pos) >= size) {
        bufflush(buffer, size, dst);
        *pos = 0;
    }
}

size_t calc_iobufsize(enum buf_type_t buftype, size_t fallback_size) {
    struct stat s;

    int tfd = -1;
    switch (buftype) {
        case BUF_STDIN:
            tfd = STDIN_FILENO;
            break;
        case BUF_STDOUT:
            tfd = STDOUT_FILENO;
            break;
    }

    int fd = fcntl(tfd, F_DUPFD, 0);
    fstat(fd, &s);
    close(fd);

    size_t bufsize = s.st_blksize;
    if (bufsize <= 0) {
        /* try to use a page size from the system */
        bufsize = sysconf(_SC_PAGESIZE);
    }

    if (bufsize <= 0) {
        bufsize = fallback_size;
    }

    return bufsize;
}
