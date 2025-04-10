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
    fprintf(stderr, "     -v <static-value>          Static value to map to (implies -z)\n\n");
    fprintf(stderr, "     --value-file <file-path>   Read map value from file (implies -z)\n\n");
    fprintf(stderr, "     --value-cmd                Use output from command as map value.\n");
    fprintf(stderr, "                                Each mapped item will be appended to the command arguments list, unless -z is specified\n");
    fprintf(stderr, "\nOptional arguments:\n");
    fprintf(stderr, "     -s <separator>             Separator character (default: '\\n')\n");
    fprintf(stderr, "     -c <concatenator>          Concatenator character (default: same as separator)\n");
    fprintf(stderr, "     -z, --discard-input        Exclude input value from map output\n");
    fprintf(stderr, "     -I <replstr>               Specifies a replacement pattern string. When used, it overrides -z.\n");
    fprintf(stderr,"                                 When the pattern is found in the map value, it is replaced with the current item from the input.\n\n");
    fprintf(stderr, "     -h, --help                 Show this help message\n");
}

void _parse_single_char_arg(char *arg, char *concat_arg, char opt_id, char *argv[]) {
    if (strlen(arg) > 1) {
        fprintf(stderr, "Error: the -%c argument must be a single character\n", opt_id);
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
        {"discard-input", no_argument, 0, 'z'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(*argc, *argv, "zs:c:v:I:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                if (map_config->source_type == MAP_VALUE_SOURCE_CMD || map_config->source_type == MAP_VALUE_SOURCE_FILE) {
                    fprintf(stderr, "-v: Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->vstatic = optarg;
                map_config->source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
                map_config->stripinput_flag = 1;
                break;
            case 'f': /* --value-file option */
                if (map_config->source_type == MAP_VALUE_SOURCE_CMD || map_config->source_type == MAP_VALUE_SOURCE_CMDLINE_ARG) {
                    fprintf(stderr, "--value-file: Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->vfpath = optarg;
                map_config->source_type = MAP_VALUE_SOURCE_FILE;
                map_config->stripinput_flag = 1;

                assert_faccessible(optarg);
                break;
            case 'r': /* --value-cmd */
                map_config->source_type = MAP_VALUE_SOURCE_CMD;
                break;
            case 'I': /* -I <replstr> */
                map_config->replstr = optarg;
                map_config->stripinput_flag = 1;
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
        map_config->cmd_argc = *argc;
        map_config->cmd_argv = *argv;
    }
}