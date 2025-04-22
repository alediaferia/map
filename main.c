#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "buffers.h"
#include "options.h"
#include "map.h"

int main(int argc, char *argv[]) {
    map_config_t map_config;
    map_config_init(&map_config);
    map_config_load_from_args(&map_config, &argc, &argv);

    /* Handle value from file if specified */
    if (map_config.vsource_t == MAP_VALUE_SOURCE_UNSPECIFIED) {
        /* Neither -v nor --value-file nor --value-cmd specified */
        fprintf(stderr, "Error: Either -v or --value-file or --value-cmd must be explicitly specified\n");
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    /* defaulting the concatenation argument to the separator one if unspecified */
    if (map_config.concatenator == 0) {
        map_config.concatenator = map_config.separator;
    }
        
    size_t bufsize = calc_iobufsize();

    char *buffer = calloc(bufsize, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Unable to allocate buffer. Check that your system has enough memory (%ld bytes)", bufsize);
	    exit(EXIT_FAILURE);
    }

    char *obuffer = calloc(bufsize, sizeof(char));
    if (obuffer == NULL) {
        fprintf(stderr, "Unable to allocate buffer. Check that your system has enough memory (%ld bytes)", bufsize);
	    exit(EXIT_FAILURE);
    }

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

    // chunk input by bufsize
    while ((bytes_read = fread(buffer, sizeof(char), bufsize, stdin)) > 0) {
        int eoiflag = 0;
        size_t i = 0;
        int mapflag = 0;
        int prev_spos = 0;

        for (; i < bytes_read; i++) {
            domap:

            eoiflag = (i == bytes_read - 1) && feof(stdin);
            // produce the map value to the output buffer
            if (buffer[i] == map_config.separator || eoiflag == 1) {
                mapflag = 1;

                size_t ilen = i - prev_spos;
                if (ilen == 0) {
                    /* ignore the current item if empty */
                    prev_spos = i + 1;
                    continue; // move to the next item;
                } else {
                    /* save the current item (to be used if referenced in the output) */
                    map_value.item = calloc(ilen + 1, sizeof(char));
                    if (map_value.item == NULL) {
                        fprintf(stderr, "Error: unable to allocate memory (%zu bytes): %s. Aborting\n", ilen, strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    memcpy(map_value.item, buffer + prev_spos, ilen * sizeof(char));

                    prev_spos = i + 1;
                }

                // load the map value
                map_vload(&map_config, &map_value);
                size_t available = bufsize - obuffer_pos;
                while (1) {
                    if (available <= 0) {
                        // no more output buffer available: flush it
                        bufflush(obuffer, bufsize, stdout);
                        obuffer_pos = 0;
                        available = bufsize;

                        if (available == 0) {
                            fprintf(stderr, "Error: no output space available.\n");
                            exit(EXIT_FAILURE);
                        }
                    }

                    size_t mapped = map_vread(obuffer + obuffer_pos, available, &map_config, &map_value);
                    available -= mapped;
                    obuffer_pos += mapped;
                    if (map_verr(&map_config, &map_value) > 0) {
                        fprintf(stderr, "Unable to write map value\n");
                        exit(EXIT_FAILURE);
                    } else if (map_veof(&map_config, &map_value) > 0) {
                        map_vreset(&map_config, &map_value);
                        break; // we have mapped the value fully, so can break
                    }
                } // map value writing cycle

                if (eoiflag == 0) {
                    // write the concatenator if this is not the end of input
                    if (available < 1) {
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
                buffer = realloc(buffer, newsize);
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
