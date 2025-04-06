#include "options.h"
#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] <value-source> [--] [cmd]\n\n", argv[0]);
    fprintf(stderr, "Value sources (one required):\n");
    fprintf(stderr, "     -v <static-value>          Static value to map to\n");
    fprintf(stderr, "     --value-file <file-path>   Read map value from file\n");
    fprintf(stderr, "     --value-cmd               Use output from command as map value\n");
    fprintf(stderr, "\nOptional arguments:\n");
    fprintf(stderr, "     -s <separator>             Separator character (default: '\\n')\n");
    fprintf(stderr, "     -c <concatenator>          Concatenator character (default: same as separator)\n");
    fprintf(stderr, "     --strip-input             Exclude input value from map output\n");
    fprintf(stderr, "     -h, --help                Show this help message\n");
}

void _parse_single_char_arg(char *arg, char *concat_arg, char opt_id, char *argv[]) {
    if (strlen(arg) > 1) {
        fprintf(stderr, "Error: the -%c argument must be a single charaacter\n", opt_id);
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    *concat_arg = arg[0];
}

void load_config_from_options(map_config_t *map_config, int *argc, char **argv[]) {
    int opt;

    /* Define long options */
    static struct option long_options[] = {
        {"value-file", required_argument, 0, 'f'},
        {"value-cmd", no_argument, 0, 'r'},
        {"strip-input", no_argument, 0, 'z'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(*argc, *argv, "s:c:v:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                if (map_config->source_type == MAP_VALUE_SOURCE_CMD || map_config->source_type == MAP_VALUE_SOURCE_FILE) {
                    fprintf(stderr, "-v: Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->vstatic = optarg;
                map_config->source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
                break;
            case 'f': /* --value-file option */
                if (map_config->source_type == MAP_VALUE_SOURCE_CMD || map_config->source_type == MAP_VALUE_SOURCE_CMDLINE_ARG) {
                    fprintf(stderr, "--value-file: Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->vfpath = optarg;
                map_config->source_type = MAP_VALUE_SOURCE_FILE;
                assert_faccessible(optarg);
                break;
            case 'r': /* --value-cmd */
                map_config->source_type = MAP_VALUE_SOURCE_CMD;
                break;
            case 's':
                _parse_single_char_arg(optarg, &(map_config->separator), opt, *argv);
                break;
            case 'c':
                _parse_single_char_arg(optarg, &(map_config->concatenator), opt, *argv);
                break;
            case 'z':
                map_config->stripinput_flag = 1;
                break;
            case '?':
            default:
                print_usage(*argv);
                exit(EXIT_FAILURE);
        }
    }
    *argc -= optind;
    *argv += optind;

    if (map_config->source_type == MAP_VALUE_SOURCE_CMD) {
        if (map_config->stripinput_flag != 1) {
            /* making room for one more input argument */
            char **argve = calloc((*argc) + 1, sizeof(char*));
            if (argve == NULL) {
                fprintf(stderr, "Error: unable to allocate memory: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            memcpy(argve, *argv, *argc);

            map_config->cmd_argc = (*argc) + 1;
            map_config->cmd_argv = argve;
        } else {
            map_config->cmd_argc = *argc;
            map_config->cmd_argv = *argv;
        }
    }
}