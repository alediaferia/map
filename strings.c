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

static inline void fill_skip_table(int *t, const char *pattern, size_t pattern_length) {
    for (int i = 0; i < 256; i++) {
        t[i] = pattern_length;
    }

    for (int i = 0; i < pattern_length - 1; i++) {
        t[pattern[i]] = pattern_length - 1 - i;
    }
}

/*
 * Finds the first occurrence of little in data and returns a pointer to it.
 * little must be 0-terminated.
 * It scans len bytes at most.
 */
static const char *strfind(const char *data, const char *little, size_t len, int *skip_table) {
    /* Boyer–Moore–Horspool */
    size_t llen = strlen(little);

    int skip = 0;
    while (len - skip >= llen) {
        if (memcmp(data + skip, little, llen) == 0) {
            return data + skip;
        }
        skip = skip + skip_table[data[skip + llen - 1]];
    }

    return NULL;
}

typedef struct {
    char **table;
    size_t count;
    size_t cap;
} matches_table_t;

#define MATCHES_TABLE_INIT_SIZE 32
#define MATCHES_TABLE_GROWTH_FACTOR 2

static void init_matches_table(matches_table_t *t) {
    t->table = malloc(MATCHES_TABLE_INIT_SIZE * sizeof(char *));
    if (t->table == NULL) {
        perror("init_matches_table");
        exit(EXIT_FAILURE);
    }
    t->count = 0;
    t->cap = MATCHES_TABLE_INIT_SIZE;
}

static void append_match(matches_table_t *t, const char *match) {
    if (t->count + 1 >= t->cap) {
        size_t newcap = t->cap * MATCHES_TABLE_GROWTH_FACTOR;
        char **resized_table = realloc(t->table, newcap * sizeof(char *));
        if (resized_table == NULL) {
            perror("append_match");
            exit(EXIT_FAILURE);
        }
        t->cap = newcap;
        t->table = resized_table;
    }

    t->table[t->count++] = (char*)match;
}

static void free_matches_table(matches_table_t *t) {
    if (t->table != NULL) {
        free(t->table);
        t->table = NULL;
    }

    t->cap = 0;
    t->count = 0;
}

const char *strreplall(const char *src, size_t srclen, const char *replstr, const char *v) {
    size_t replstrlen = strlen(replstr);
    if (replstrlen == 0) {
        return NULL;
    }

    size_t vlen = strlen(v);
    
    int skip_table[256];
    fill_skip_table(skip_table, replstr, replstrlen);

    matches_table_t matches;
    init_matches_table(&matches);

    const char *cur = src;
    const char *match = NULL;

    while ((match = strfind(cur, replstr, srclen - (cur - src), skip_table)) != NULL) {
        append_match(&matches, match);
        cur = match + replstrlen;
    }

    size_t newsize = srclen - (replstrlen * matches.count) + (vlen * matches.count) + 1;

    char *result = malloc(newsize * sizeof(char));
    if (result == NULL) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    cur = src;
    const char *s = NULL;
    char *dst = result;

    for (int i = 0; i < matches.count; i++) {
        const char *match = matches.table[i];
        /* copy from source until match index */
        size_t p = match - cur;
        memcpy(dst, cur, p);
        dst += p; /* increment pointer to destination by data size written so far */

        /* now copy the value to replace the match */
        memcpy(dst, v, vlen);
        dst += vlen; /* advance dst by the value size */
        cur = match + replstrlen; /* advance the cursor in the source string to the end of the current match */
    }

    /* cur is now pointing right at the end of the last match (or the begginning of src): copy what's left */
    memcpy(dst, cur, srclen - (cur - src));
    result[newsize - 1] = '\0';

    free_matches_table(&matches);
    
    return result;
}
