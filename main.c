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
    fprintf(stderr, "Usage: %s [options] (-v <static-value> | --value-file <file-path> | --value-cmd) [--] [cmd]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "     -v <static-value>          Value to map to (default: none)\n");
    fprintf(stderr, "     --value-file <file-path>   Path to a file containing the value to map to\n");
    fprintf(stderr, "     --value-cmd <cmd>          The command to run to get the value to map to\n");
    fprintf(stderr, "     -s <separator>             Separator character (default: '\\n')\n");
    fprintf(stderr, "     -c <concatenator>          Concatenator character (default: same as separator)\n");
    fprintf(stderr, "     -h, --help                 Show this help message\n");
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

    /* The file descriptor can be closed after mmap succeeds */
    close(fd);

    *content_length = st.st_size;
    return mapped;
}

size_t calc_stdio_buffer_size() {
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

    return bufsize;
}

#define DEFAULT_SEPARATOR_VALUE '\n'

FILE* run_map_cmd_fe(int argc, char *argv[]) {
    if (argc == 0) {
        return NULL;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) { // Child process
        close(pipefd[0]);  // Close read end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(argv[0], argv);
        // If execvp returns, there was an error
        fprintf(stderr, "Error executing command: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[1]);  // Close write end
    FILE *fp = fdopen(pipefd[0], "r");
    if (fp == NULL) {
        fprintf(stderr, "Error creating file stream: %s\n", strerror(errno));
        close(pipefd[0]);
        return NULL;
    }

    return fp;
}

FILE* run_map_cmd(int argc, char *argv[]) {
    /* reconstruct the argv as a single command line string */
    size_t cmdline_size = 0;
    for (int i = 0; i < argc; i++) {
        cmdline_size += strlen(argv[0]);
    }

    char *cmd = calloc(cmdline_size + 1, sizeof(char));
    if (cmd == NULL) {
        fprintf(stderr, "Unable to allocate buffer. Check that your system has enough memory (%ld bytes)", cmdline_size);
        exit(EXIT_FAILURE);
    }

    size_t cmd_pos = 0;
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        memcpy(cmd + cmd_pos, argv[i], len);
        cmd_pos += len;
        cmd[cmd_pos++] = ' ';
    }
    cmd[cmd_pos - 1] = '\0'; // remove the last space

    /* run the command using popen */
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot run command %s: %s\n", cmd, strerror(errno));
        free(cmd);
        return NULL;
    }

    /* free the command string */
    free(cmd);
    
    return fp;
}

enum map_value_source_type {
    MAP_VALUE_SOURCE_UNSPECIFIED = -1,
    MAP_VALUE_SOURCE_CMDLINE_ARG = 0,
    MAP_VALUE_SOURCE_FILE,
    MAP_VALUE_SOURCE_CMD
};

typedef struct map_config {
    char *map_value; // the value to map to
    size_t map_value_length;
    char *map_value_file_path;
    char separator;
    char concatenator;
    enum map_value_source_type map_value_source;
} map_config_t;

map_config_t new_map_config() {
    return (map_config_t){
        NULL,
        0,
        NULL,
        DEFAULT_SEPARATOR_VALUE,
        0,
        MAP_VALUE_SOURCE_UNSPECIFIED,
    };
}

void load_config_from_options(map_config_t *map_config, int *argc, char **argv[]) {
    int opt;

    /* Define long options */
    static struct option long_options[] = {
        {"value-file", required_argument, 0, 'f'},
        {"value-cmd", no_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(*argc, *argv, "s:c:v:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                if (map_config->map_value_source == MAP_VALUE_SOURCE_CMD || map_config->map_value_source == MAP_VALUE_SOURCE_FILE) {
                    fprintf(stderr, "Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->map_value = optarg;
                map_config->map_value_source = MAP_VALUE_SOURCE_CMDLINE_ARG;
                break;
            case 'f': /* --value-file option */
                if (map_config->map_value_source == MAP_VALUE_SOURCE_CMD || map_config->map_value_source == MAP_VALUE_SOURCE_CMDLINE_ARG) {
                    fprintf(stderr, "Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->map_value_file_path = optarg;
                map_config->map_value_source = MAP_FILE;
                break;
            case 'r': /* --value-cmd */
                map_config->map_value_source = MAP_VALUE_SOURCE_CMD;
                break;
            case 's':
                parse_single_char_arg(optarg, &(map_config->separator), opt, *argv);
                break;
            case 'c':
                parse_single_char_arg(optarg, &(map_config->concatenator), opt, *argv);
                break;
            case '?':
            default:
                print_usage(*argv);
                exit(EXIT_FAILURE);
        }
    }
    *argc -= optind;
    *argv += optind;
}

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
            map_config.map_value = read_file_content(map_config.map_value_file_path, &(map_config.map_value_length));
            if (map_config.map_value == NULL) {
                /* Error message already printed in read_file_content */
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
                    FILE *map_cmd_out = run_map_cmd_fe(cmd_argc, cmd_argv);
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
