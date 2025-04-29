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

int buffer_init(buffer_t* buffer, size_t size) {
    buffer->data = calloc(size, sizeof(char));
    if (buffer->data == NULL) {
        perror("buffer_init");
        return BUFFER_MEM_ERROR;
    }

    buffer->size = size;
    buffer->pos = 0;

    return BUFFER_SUCCESS;
}

void buffer_free(buffer_t* buffer) {
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
}

void buffer_reset(buffer_t* buffer) {
    buffer->pos = 0;
}

size_t buffer_load(buffer_t* dst, FILE* src) {
    size_t r = fread(dst->data + dst->pos, sizeof(char), dst->size - dst->pos, src);
    dst->pos += r;
    return r;
}

size_t buffer_available(buffer_t *buffer) {
    return buffer->size - buffer->pos;
}

int buffer_flush(FILE* dst, buffer_t* buffer) {
    if (fwrite(buffer->data, sizeof(char), buffer->pos, dst) < buffer->pos) {
        if (ferror(dst) > 0) {
            fprintf(stderr, "Error: bufflush: unable to flush buffer: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Error: bufflush: unable to flush buffer\n");
        }
        return BUFFER_FLUSH_ERROR;
    }

    return BUFFER_SUCCESS;
}

int buffer_extend(buffer_t* buffer, size_t newsize) {
    if (newsize < buffer->size) {
        return BUFFER_SIZE_TOO_SHORT;
    }

    if (newsize == buffer->size) {
        return BUFFER_SUCCESS;
    }

    char *newbuf = realloc(buffer->data, newsize);
    if (newbuf == NULL) {
        perror("buffer_extend");
        return BUFFER_MEM_ERROR;
    }

    buffer->data = newbuf;
    buffer->size = newsize;

    return BUFFER_SUCCESS;
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
