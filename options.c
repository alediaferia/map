#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

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
                map_config->map_value_source = MAP_VALUE_SOURCE_FILE;
                break;
            case 'r': /* --value-cmd */
                map_config->map_value_source = MAP_VALUE_SOURCE_CMD;
                break;
            case 's':
                _parse_single_char_arg(optarg, &(map_config->separator), opt, *argv);
                break;
            case 'c':
                _parse_single_char_arg(optarg, &(map_config->concatenator), opt, *argv);
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