/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: strings.c
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

#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *strreplall(const char *src, const char *replstr, const char *v) {
    size_t replstrlen = strlen(replstr);
    if (replstrlen == 0) {
        return NULL;
    }

    size_t vlen = strlen(v);

    size_t occurs = 0;
    const char *tmp = src;
    while ((tmp = strstr(tmp, replstr)) != NULL) {
        occurs += 1;
        tmp += vlen;
    }

    size_t srclen = strlen(src);
    size_t newsize = srclen - (replstrlen * occurs) + (vlen * occurs) + 1;

    char *news = calloc(newsize, sizeof(char));
    if (news == NULL) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    tmp = src;
    char *s = NULL;
    char *dst = news;
    while ((s = strstr(tmp, replstr)) != NULL) {
        size_t len = s - tmp;
        memcpy(dst, tmp, len);
        dst += len;
        memcpy(dst, v, vlen);
        dst += vlen;
        tmp = s + replstrlen;
    }

    if ((size_t)(tmp - src) < srclen) {
        strcpy(dst, tmp);
    }
    
    return news;
}
