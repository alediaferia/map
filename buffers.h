/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: buffers.h
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

#ifndef BUFFERS_H
#define BUFFERS_H

#include <unistd.h>
#include <stdio.h>

enum buf_type_t {
    BUF_STDIN,
    BUF_STDOUT
};

typedef enum {
    BUFFER_SUCCESS = 0,
    BUFFER_MEM_ERROR,
    BUFFER_FLUSH_ERROR,
    BUFFER_SIZE_TOO_SHORT
} buffer_result_t;

typedef struct {
    char *data;
    int pos;
    size_t size;
} buffer_t;

int buffer_init(buffer_t *buffer, size_t size);
void buffer_free(buffer_t *buffer);
void buffer_reset(buffer_t *buffer);
size_t buffer_load(buffer_t *dst, FILE *src);
size_t buffer_available(buffer_t *buffer);
int buffer_flush(FILE *dst, buffer_t *buffer);
int buffer_extend(buffer_t *buffer, size_t newsize);

/*
 * Computes a buffer size appropriate on the current system
 * and returns fallback_size if an appropriate size cannot be determined.
 */
size_t calc_iobufsize(enum buf_type_t buftype, size_t fallback_size);

#endif // BUFFERS_H
