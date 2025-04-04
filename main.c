#include <string.h>

#include "config.h"
#include "options.h"
#include "cmd.h"
#include "files.h"
#include "map.h"

int main(int argc, char *argv[]) {
    map_config_t map_config = new_map_config();
    load_config_from_options(&map_config, &argc, &argv);

    /* Handle value from file if specified */
    switch (map_config.source_type) {
        case MAP_VALUE_SOURCE_UNSPECIFIED:
            /* Neither -v nor --value-file nor --value-cmd specified */
            fprintf(stderr, "Error: Either -v or --value-file or --value-cmd must be explicitly specified\n");
            print_usage(argv);
            exit(EXIT_FAILURE);
        case MAP_VALUE_SOURCE_FILE:
            map_config.static_value = mmap_file(map_config.map_value_file_path, &(map_config.map_value_length));
            if (map_config.static_value == NULL) {
                /* Error message already printed in mmap_file */
                exit(EXIT_FAILURE);
            }
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            map_config.map_value_length = strlen(map_config.static_value);
            break;
        case MAP_VALUE_SOURCE_CMD:
            map_config.cmd_argc = argc;
            map_config.cmd_argv = argv;
            break;
    }

    /* defaulting the concatenation argument to the separator one if unspecified */
    if (map_config.concatenator == 0) {
        map_config.concatenator = map_config.separator;
    }
        
    size_t bufsize = calc_stdio_buffer_size();

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
    size_t last_separator_pos = 0; /* input item offset (one item per separator expected) */
    map_value_source_t value = new_map_value_source();

    while ((bytes_read = fread(buffer, sizeof(char), bufsize, stdin)) > 0) {
        /* scan the input for the separator char */
        for (size_t i = 0; i < bytes_read; i++) {
            int eoi = 0;
            if (buffer[i] == map_config.separator || (eoi = (i == bytes_read - 1)) == 1) {
                if (i - last_separator_pos <= 1) {
                    /* no content between this and previous separator: skipping */
                    last_separator_pos = i;
                    continue;
                } else {
                    last_separator_pos = i;
                }

                mvload(&map_config, &value);
                size_t available = bufsize - obuffer_pos;
                if (available <= 0) {
                    /* Not enough space for the map value, flush buffer */
                    if (fwrite(obuffer, sizeof(char), obuffer_pos, stdout) != obuffer_pos) {
                        fprintf(stderr, "Error writing to stdout\n");
                        free(buffer);
                        free(obuffer);
                        exit(EXIT_FAILURE);
                    }
                    obuffer_pos = 0;
                    available = bufsize;
                } 
                size_t read_len = mvread(obuffer + obuffer_pos, available, &map_config, &value);
                while (1) {
                    if (read_len == 0) {
                        mvreset(&map_config, &value);
                        break;
                    }

                    obuffer_pos += read_len;
                    available -= read_len;
                    if (obuffer_pos >= bufsize) {
                        /* Time to flush buffer */
                        if (fwrite(obuffer, sizeof(char), obuffer_pos, stdout) != obuffer_pos) {
                            fprintf(stderr, "Error writing to stdout\n");
                            free(buffer);
                            free(obuffer);
                            mvclose(&map_config, &value);
                            exit(EXIT_FAILURE);
                        }
                        obuffer_pos = 0;
                        available = bufsize;
                    }

                    if (mveof(&map_config, &value) != 0) {
                        mvreset(&map_config, &value);
                        break;
                    } else if (mverr(&map_config, &value) != 0) {
                        fprintf(stderr, "Error reading from map value\n");
                        free(buffer);
                        free(obuffer);
                        mvclose(&map_config, &value);
                        exit(EXIT_FAILURE);
                    }
                    /* read more data from the command output */
                    mvread(obuffer + obuffer_pos, available, &map_config, &value);
                }

                /* for now, we arbitrarily avoid writing the concatenator as the last character */
                if (eoi == 0) {
                    /* now time to write the concatenator char */
                    if (obuffer_pos + 1 >= bufsize) {
                        /* Not enough space for concatenator, flush buffer */
                        if (fwrite(obuffer, sizeof(char), obuffer_pos, stdout) != obuffer_pos) {
                            fprintf(stderr, "Error writing to stdout\n");
                            free(buffer);
                            free(obuffer);
                            exit(EXIT_FAILURE);
                        }
                        obuffer_pos = 0;
                    }
                    obuffer[obuffer_pos++] = map_config.concatenator;
                }
            }
        }

        /* Check for errors */
        if (ferror(stdin)) {
            fprintf(stderr, "Error reading from stdin\n");
            free(buffer);
            free(obuffer);
            mvclose(&map_config, &value); 
            exit(EXIT_FAILURE);
        }
    }

    /* Flush any remaining data in the output buffer */
    if (obuffer_pos > 0) {
        if (fwrite(obuffer, sizeof(char), obuffer_pos, stdout) != obuffer_pos) {
            fprintf(stderr, "Error writing to stdout\n");
            free(buffer);
            free(obuffer);
            mvclose(&map_config, &value);      
            exit(EXIT_FAILURE);
        }
    }

    free(buffer);
    free(obuffer);
    mvclose(&map_config, &value);

    exit(EXIT_SUCCESS);
}
