/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: options.c
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

#include "options.h"
#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] <value-source-modifier> [--] [cmd]\n\n", argv[0]);
    fprintf(stderr, "Available value source modifiers:\n");
    fprintf(stderr, "     -v <static-value>          Static value to map to (implies -z)\n\n");
    fprintf(stderr, "     --value-file <file-path>   Read map value from file (implies -z)\n\n");
    fprintf(stderr, "     --value-cmd                Use output from command as map value\n");
    fprintf(stderr, "                                Each mapped item will be appended to the command arguments list, unless -z is specified\n");
    fprintf(stderr, "\nOptional arguments:\n");
    fprintf(stderr, "     -s <separator>             Separator character (default: '\\n')\n");
    fprintf(stderr, "     -c <concatenator>          Concatenator character (default: same as separator)\n");
    fprintf(stderr, "     -z, --discard-input        Exclude input value from map output\n");
    fprintf(stderr, "     -I <replstr>               Specifies a replacement pattern string. When used, it overrides -z.\n");
    fprintf(stderr, "                                When the pattern is found in the map value, it is replaced with the current item from the input.\n\n");
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

void map_config_load_from_args(map_config_t *map_config, int *argc, char **argv[]) {
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
                if (map_config->vsource_t == MAP_VALUE_SOURCE_CMD || map_config->vsource_t == MAP_VALUE_SOURCE_FILE) {
                    fprintf(stderr, "-v: Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->vstatic = optarg;
                map_config->vsource_t = MAP_VALUE_SOURCE_CMDLINE_ARG;
                break;
            case 'f': /* --value-file option */
                if (map_config->vsource_t == MAP_VALUE_SOURCE_CMD || map_config->vsource_t == MAP_VALUE_SOURCE_CMDLINE_ARG) {
                    fprintf(stderr, "--value-file: Error: you can only specify one value mapping option (-v or --value-file or --value-cmd)\n");
                    print_usage(*argv);
                    exit(EXIT_FAILURE);
                }
                map_config->vfpath = optarg;
                map_config->vsource_t = MAP_VALUE_SOURCE_FILE;
                map_config->stripi_f = 1;

                assert_faccessible(optarg);
                break;
            case 'r': /* --value-cmd */
                map_config->vsource_t = MAP_VALUE_SOURCE_CMD;
                break;
            case 'I': /* -I <replstr> */
                map_config->replstr = optarg;
                map_config->stripi_f = 0;
                break;
            case 's':
                _parse_single_char_arg(optarg, &(map_config->separator), opt, *argv);
                break;
            case 'c':
                _parse_single_char_arg(optarg, &(map_config->concatenator), opt, *argv);
                break;
            case 'z':
                map_config->stripi_f = 1;
                break;
            case '?':
            default:
                print_usage(*argv);
                exit(EXIT_FAILURE);
        }
    }
    *argc -= optind;
    *argv += optind;

    if (map_config->vsource_t == MAP_VALUE_SOURCE_CMD) {
        map_config->cmd_argc = *argc;
        map_config->cmd_argv = *argv;
    }
}
