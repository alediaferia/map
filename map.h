/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: map.h
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

#ifndef MAP_H
#define MAP_H

#include <stdio.h>

typedef struct map_value {
    union {
        /* stream holding the map value (if not in msource) */
        FILE *fsource;

        /* pointer to the map value in memory (if not in fsource) */
        const char *msource;
    };

    /* length of the map value (if not msource) */
    size_t mlen;

    /* tracks the last byte from msource written to the destination */
    size_t pos;

    /* the input item to map: needed when the map value references the input item */
    char *item;
} map_value_t;

enum map_vsource {
    MAP_VALUE_SOURCE_UNSPECIFIED = -1,
    MAP_VALUE_SOURCE_CMDLINE_ARG = 0,
    MAP_VALUE_SOURCE_FILE,
    MAP_VALUE_SOURCE_CMD
};

typedef struct map_config {
    union {
        const char *vstatic;
        const char *vfpath;
    };

    char separator;
    char concatenator;

    enum map_vsource vsource_t;

    int cmd_argc;
    char **cmd_argv;

    /* strip input flag */
    int stripi_f;

    const char *replstr;
} map_config_t;

void map_value_init(map_value_t *v);
void map_config_init(map_config_t *c);

/*
 * Copies len bytes of the given src into v to be later used for mapping operations.
 * The copied sequence of bytes will be null-terminated.
 */
void map_vicpy(map_value_t *v, const char *src, size_t len);

/*
    Copies at most max_len bytes of src into dst.
    Any occurrences of config->replstr will be replaced with src->item
    if both are present.

    note: src must have been initialized using map_vload.
 */
size_t map_vread(char *dst, size_t max_len, const map_config_t *config, map_value_t *v);

/*
 * Returns 1 if the source v has been consumed fully.
 */
int map_veof(const map_config_t *config, const map_value_t *v);

/*
 * Returns 1 if the underlying source v stream reports an error. 
 * Returns 0 in all other cases.
 */
int map_verr(const map_config_t *config, const map_value_t *v);

/*
 * Closes any stream associated with v and resets the internal offset.
 */
void map_vclose(const map_config_t *config, map_value_t *v);

/*
 * Resets the value source so that the next read will start from the beginning
 * of the value.
 * 
 * If the value source is of type MAP_VALUE_SOURCE_CMD, this command
 * will pclose the underlying stream.
 * 
 * In the other cases, it will reset the read offset to 0 only.
 * 
 * You should use map_vclose if you do not plan to read from v again.
 */
void map_vreset(const map_config_t *config, map_value_t *v);

/*
 * Loads the value source as per the type specified in config.
 * Exits the program upon failure or validation issue.
 */
void map_vload(const map_config_t *config, map_value_t *v);

#endif // MAP_H
