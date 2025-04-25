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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "options.h"
#include "map.h"

static char* bufalloc(size_t size) {
    char *buffer = calloc(size, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Unable to allocate buffer. Check that your system has enough memory (%ld bytes)", size);
        exit(EXIT_FAILURE);
    }

    return buffer;
}

static void init_from_opts(map_config_t *config, int *argc, char ***argv) {
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

int main(int argc, char *argv[]) {
    map_config_t map_config;
    init_from_opts(&map_config, &argc, &argv);
        
    size_t bufsize = calc_iobufsize();
    assert(bufsize > 0);

    char *buffer = bufalloc(bufsize);
    char *obuffer = bufalloc(bufsize);

    /* 
        We are going to read from stdin to a buffer, for efficiency purposes.
        Whenever we encounter the separator char, we will produce the map value
        to the output buffer.
        Whenever the output buffer fills up, we are going to flush it to stdout.
    */

    size_t bytes_read;
    size_t obuffer_pos = 0;

    map_value_t map_value;
    map_value_init(&map_value);

    /*
        read from stdin into the buffer
        if the data read so far includes the separator character or this is the end of the input
            map it to the map value and write it to the output buffer
        else extend the buffer to read more and repeat
    */

    while ((bytes_read = fread(buffer, sizeof(char), bufsize, stdin)) > 0) {
        int eoiflag = 0;
        size_t i = 0;
        int mapflag = 0;
        int prev_spos = 0;

        for (; i < bytes_read; i++) {
            domap:

            eoiflag = (i == bytes_read - 1) && feof(stdin);
            if (buffer[i] == map_config.separator || eoiflag == 1) {
                mapflag = 1;

                size_t itemlen = i - prev_spos;
                if (itemlen == 0) {
                    /* ignore the current item if empty */
                    prev_spos = i + 1;
                    continue;
                } else {
                    /* save the current item (to be used if referenced in the output) */
                    map_vicpy(&map_value, buffer + prev_spos, itemlen);
                    prev_spos = i + 1;
                }

                map_vload(&map_config, &map_value);
                size_t available = bufsize - obuffer_pos;

                /* loop to write out the mapped value to the output buffer until done */
                while (1) {
                    if (available <= 0) {
                        bufflush(obuffer, bufsize, stdout);
                        obuffer_pos = 0;
                        available = bufsize;
                    }

                    size_t mapped = map_vread(obuffer + obuffer_pos, available, &map_config, &map_value);
                    available -= mapped;
                    obuffer_pos += mapped;

                    if (map_verr(&map_config, &map_value) > 0) {
                        fprintf(stderr, "Unable to write map value\n");
                        exit(EXIT_FAILURE);
                    } 

                    if (map_veof(&map_config, &map_value) > 0) {
                        map_vreset(&map_config, &map_value);
                        break;
                    }
                }

                if (eoiflag == 0) {
                    // write the concatenator if this is not the end of input
                    if (available <= 0) {
                        bufflush(obuffer, bufsize, stdout);
                        obuffer_pos = 0;
                        available = bufsize;
                    }
                    obuffer[obuffer_pos++] = map_config.concatenator;
                }
            }

            if (map_value.item != NULL) {
                free(map_value.item);
                map_value.item = NULL;
            }
        }

        if (mapflag == 0) {
            /* 
                we haven't mapped the current input to anything yet
                because we haven't found a separator character. 

                We cannot through the current buffer away so we should extend
                it if we expect more input coming in.
            */
            if (!feof(stdin)) {
                size_t newsize = bufsize * 2;
                char *newbuf = realloc(buffer, newsize);
                if (newbuf == NULL) {
                    fprintf(stderr, "Failed to allocate memory: unable to extend input buffer. Aborting.\n");
                    free(buffer);
                    free(obuffer);
                    map_vclose(&map_config, &map_value);
                    exit(EXIT_FAILURE);
                }
                buffer = newbuf;
                bytes_read += fread(buffer + bufsize, sizeof(char), newsize - bufsize, stdin);
                bufsize = newsize;
                goto domap;
            }
        }
    }

    /* Flush any remaining data in the output buffer */
    if (obuffer_pos > 0) {
        bufflush(obuffer, obuffer_pos, stdout);
    }

    free(buffer);
    free(obuffer);
    map_vclose(&map_config, &map_value);

    exit(EXIT_SUCCESS);
}
