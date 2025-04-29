/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: main.c
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

#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "options.h"
#include "map.h"

#define FALLBACK_BUFFER_SIZE 4069
#define BUFFER_INCREASE_FACTOR 2

static inline void init_from_opts(map_config_t *config, int *argc, char ***argv) {
    map_config_init(config);
    map_config_load_from_args(config, argc, argv);

    /* Handle value from file if specified */
    if (config->vsource_t == MAP_VALUE_SOURCE_UNSPECIFIED) {
        /* Neither -v nor --value-file nor --value-cmd specified */
        fprintf(stderr, "Error: One of -v, --value-file or --value-cmd must be explicitly specified\n");
        print_usage(*argv);
        exit(EXIT_FAILURE);
    }

    /* defaulting the concatenation argument to the separator one if unspecified */
    if (config->concatenator == 0) {
        config->concatenator = config->separator;
    }
}

static inline int init_buffers(buffer_t *ibuffer, buffer_t *obuffer) {
    if (buffer_init(ibuffer, calc_iobufsize(BUF_STDIN, FALLBACK_BUFFER_SIZE)) != BUFFER_SUCCESS) {
        return -1;
    }

    if (buffer_init(obuffer, calc_iobufsize(BUF_STDIN, FALLBACK_BUFFER_SIZE)) != BUFFER_SUCCESS) {
        return -1;
    } 

    return 0;
}

int main(int argc, char *argv[]) {
    int exit_code = EXIT_SUCCESS;

    map_config_t map_config;
    init_from_opts(&map_config, &argc, &argv);

    size_t bytes_read = 0;

    map_value_t map_value;
    map_value_init(&map_value);

    buffer_t buf;
    buffer_t obuf;

    if (init_buffers(&buf, &obuf) != 0) {
        fprintf(stderr, "Unable to initialize buffer. Aborting.\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    /*
        read from stdin into the buffer
        if the data read so far includes the separator character or this is the end of the input
            map it to the map value and write it to the output buffer
        else extend the buffer to read more and repeat
    */

    while ((bytes_read = buffer_load(&buf, stdin)) > 0) {
        int eoiflag = 0;
        size_t i = 0;
        int mapflag = 0;
        int prev_spos = 0;

        for (; i < bytes_read; i++) {
            eoiflag = (i == bytes_read - 1) && feof(stdin);
            if (buf.data[i] == map_config.separator || eoiflag == 1) {
                mapflag = 1;

                size_t itemlen = i - prev_spos;
                if (itemlen == 0) {
                    /* ignore the current item if empty */
                    prev_spos = i + 1;
                    continue;
                } else {
                    /* save the current item (to be used if referenced in the output) */
                    map_vicpy(&map_value, buf.data + prev_spos, itemlen);
                    prev_spos = i + 1;
                }

                map_vload(&map_config, &map_value);

                /* loop to write out the mapped value to the output buffer until done */
                while (map_veof(&map_config, &map_value) <= 0) {
                    buffer_flush(stdout, &obuf);
                    buffer_reset(&obuf);

                    size_t mapped = map_vread(obuf.data + obuf.pos, obuf.size - obuf.pos, &map_config, &map_value);
                    obuf.pos += mapped;

                    if (map_verr(&map_config, &map_value) > 0) {
                        fprintf(stderr, "Unable to write map value\n");
                        exit_code = EXIT_FAILURE;
                        goto cleanup;
                    }
                }
                map_vreset(&map_config, &map_value);

                if (eoiflag == 0) {
                    buffer_flush(stdout, &obuf);
                    buffer_reset(&obuf);
                    obuf.data[obuf.pos++] = map_config.concatenator;
                }
            }

            if (map_value.item != NULL) {
                free(map_value.item);
                map_value.item = NULL;
            }

            if (i == bytes_read - 1 && mapflag == 0 && !feof(stdin)) {
                /* we have not found any separator character yet */
                if (buffer_extend(&buf, buf.size * BUFFER_INCREASE_FACTOR) != BUFFER_SUCCESS) {
                    fprintf(stderr, "Failed to allocate memory: unable to extend input buffer. Aborting.\n");
                    exit_code = EXIT_FAILURE;
                    goto cleanup;
                }

                bytes_read += buffer_load(&buf, stdin);
            }
        }

        buffer_reset(&buf);
    }

    /* Flush any remaining data in the output buffer */
    buffer_flush(stdout, &obuf);

cleanup:
    buffer_free(&obuf);
    buffer_free(&buf);
    map_vclose(&map_config, &map_value);

    return exit_code;
}
