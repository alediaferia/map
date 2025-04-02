#include <string.h>

#include "config.h"
#include "options.h"
#include "cmd.h"
#include "files.h"


int main(int argc, char *argv[]) {
    map_config_t map_config = new_map_config();

    load_config_from_options(&map_config, &argc, &argv);

    int cmd_argc = 0;
    char **cmd_argv = NULL;

    /* Handle value from file if specified */
    switch (map_config.map_value_source) {
        case MAP_VALUE_SOURCE_UNSPECIFIED:
            /* Neither -v nor --value-file nor --value-cmd specified */
            fprintf(stderr, "Error: Either -v or --value-file or --value-cmd must be explicitly specified\n");
            print_usage(argv);
            exit(EXIT_FAILURE);
        case MAP_VALUE_SOURCE_FILE:
            map_config.map_value = mmap_file(map_config.map_value_file_path, &(map_config.map_value_length));
            if (map_config.map_value == NULL) {
                /* Error message already printed in mmap_file */
                exit(EXIT_FAILURE);
            }
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            map_config.map_value_length = strlen(map_config.map_value);
            break;
        case MAP_VALUE_SOURCE_CMD:
            cmd_argc = argc;
            cmd_argv = argv;
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

    while ((bytes_read = fread(buffer, sizeof(char), bufsize, stdin)) > 0) {
        /* scan the input for the separator char */
        for (size_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == map_config.separator) {
                if (map_config.map_value_source == MAP_VALUE_SOURCE_CMD) { /* different buffer treatment (for now) */
                    FILE *map_cmd_out = runcmd(cmd_argc, cmd_argv);
                    if (map_cmd_out == NULL) {
                        /* Error message already printed in run_map_cmd */
                        free(buffer);
                        free(obuffer);
                        exit(EXIT_FAILURE);
                    }

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
                    size_t read_len = fread(obuffer + obuffer_pos, sizeof(char), available, map_cmd_out);
                    while (1) {
                        if (read_len == 0) {
                            pclose(map_cmd_out);
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
                                pclose(map_cmd_out);
                                exit(EXIT_FAILURE);
                            }
                            obuffer_pos = 0;
                            available = bufsize;
                        }

                        if (feof(map_cmd_out) != 0) {
                            pclose(map_cmd_out);
                            break;
                        } else if (ferror(map_cmd_out) != 0) {
                            fprintf(stderr, "Error reading from command output\n");
                            free(buffer);
                            free(obuffer);
                            pclose(map_cmd_out);
                            exit(EXIT_FAILURE);
                        }
                        /* read more data from the command output */
                        read_len = fread(obuffer + obuffer_pos, sizeof(char), available, map_cmd_out);
                    }
                } else {
                    size_t remaining = map_config.map_value_length;
                    while (remaining > 0) {
                        size_t len = (remaining < bufsize - obuffer_pos) ? remaining : bufsize - obuffer_pos;
                        if (len == 0) {
                            /* we have reached the end of the buffer: time to flush */
                            if (fwrite(obuffer, sizeof(char), obuffer_pos, stdout) != obuffer_pos) {
                                fprintf(stderr, "Error writing to stdout\n");
                                free(buffer);
                                free(obuffer);
                                exit(EXIT_FAILURE);
                            }
                            obuffer_pos = 0;
                        } else {
                            memcpy(obuffer + obuffer_pos, map_config.map_value, len);
                            obuffer_pos += len;
                            remaining -= len;
                        }
                    }
                }

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

        /* Check for errors */
        if (ferror(stdin)) {
            fprintf(stderr, "Error reading from stdin\n");
            free(buffer);
            free(obuffer);
            exit(EXIT_FAILURE);
        }
    }

    /* Flush any remaining data in the output buffer */
    if (obuffer_pos > 0) {
        if (fwrite(obuffer, sizeof(char), obuffer_pos, stdout) != obuffer_pos) {
            fprintf(stderr, "Error writing to stdout\n");
            free(buffer);
            free(obuffer);
            exit(EXIT_FAILURE);
        }
    }

    free(buffer);
    free(obuffer);

    exit(EXIT_SUCCESS);
}
