#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>
#include <getopt.h>
#include <errno.h>

#define FALLBACK_BUFFER_SIZE 4069

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [-c concatenator] [-s separator] (-v <static-value> | --value-file <file-path>)\n", argv[0]);
}

void parse_single_char_arg(char *arg, char *concat_arg, char opt_id, char *argv[]) {
    if (strlen(arg) > 1) {
        fprintf(stderr, "Error: the -%c argument must be a single charaacter\n", opt_id);
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    *concat_arg = arg[0];
}

void* read_file_content(const char *filepath, size_t *content_length) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error: Cannot open file %s: %s\n", filepath, strerror(errno));
        return NULL;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Error: Cannot stat file %s: %s\n", filepath, strerror(errno));
        close(fd);
        return NULL;
    }

    if (st.st_size <= 0) {
        fprintf(stderr, "Error: File %s is empty\n", filepath);
        close(fd);
        return NULL;
    }

    // Map the file into memory
    void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        fprintf(stderr, "Error: Cannot mmap file %s: %s\n", filepath, strerror(errno));
        close(fd);
        return NULL;
    }

    // The file descriptor can be closed after mmap succeeds
    close(fd);

    *content_length = st.st_size;
    return mapped;
}

int main(int argc, char *argv[]) {
    int opt;
    char *s_value_arg = NULL;
    char *value_file_path = NULL;
    char s_concat_arg = 0;
    char s_sep_arg = '\n';
    int value_from_file = 0;
    size_t s_value_arg_len = 0;

    /* Define long options */
    static struct option long_options[] = {
        {"value-file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "s:c:v:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                if (value_from_file) {
                    fprintf(stderr, "Error: Cannot specify both -v and --value-file\n");
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                s_value_arg = optarg;
                break;
            case 'f': /* --value-file option */
                if (s_value_arg != NULL) {
                    fprintf(stderr, "Error: Cannot specify both -v and --value-file\n");
                    print_usage(argv);
                    exit(EXIT_FAILURE);
                }
                value_file_path = optarg;
                value_from_file = 1;
                break;
            case 's':
                parse_single_char_arg(optarg, &s_sep_arg, opt, argv);
                break;
            case 'c':
                parse_single_char_arg(optarg, &s_concat_arg, opt, argv);
                break;
            case '?':
            default:
                print_usage(argv);
                exit(EXIT_FAILURE);
        }
    }

    /* Handle value from file if specified */
    if (value_from_file) {
        s_value_arg = read_file_content(value_file_path, &s_value_arg_len);
        if (s_value_arg == NULL) {
            /* Error message already printed in read_file_content */
            exit(EXIT_FAILURE);
        }
    } else if (s_value_arg == NULL) {
        /* Neither -v nor --value-file specified */
        fprintf(stderr, "Error: Either -v or --value-file must be specified\n");
        print_usage(argv);
        exit(EXIT_FAILURE);
    } else {
        s_value_arg_len = strlen(s_value_arg);
    }

    if (s_concat_arg == 0) {
        s_concat_arg = s_sep_arg;
    }
        
    /* calculate the optimal buffer size for stdout/stdin */ 
    struct stat s;
    int fd = fcntl(STDIN_FILENO, F_DUPFD, 0);
    fstat(fd, &s);
    close(fd);
    size_t bufsize = s.st_blksize;
    if (bufsize <= 0) {
        /* try to use a page size from the system */
        bufsize = sysconf(_SC_PAGESIZE);
        if (bufsize <= 0) {
            bufsize = FALLBACK_BUFFER_SIZE; // arbitrary fall back
        }
    }

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
            if (buffer[i] == s_sep_arg) {
                size_t remaining = s_value_arg_len;
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
                        memcpy(obuffer + obuffer_pos, s_value_arg, len);
                        obuffer_pos += len;
                        remaining -= len;
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
                obuffer[obuffer_pos++] = s_concat_arg;
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
