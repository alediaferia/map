#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdio.h>

#define DEFAULT_SEPARATOR_VALUE '\n'
#define FALLBACK_BUFFER_SIZE 4069

enum map_value_source_type {
    MAP_VALUE_SOURCE_UNSPECIFIED = -1,
    MAP_VALUE_SOURCE_CMDLINE_ARG = 0,
    MAP_VALUE_SOURCE_FILE,
    MAP_VALUE_SOURCE_CMD
};


typedef struct map_config {
    char *static_value; // the value to map to
    char *map_value_file_path;
    char separator;
    char concatenator;
    enum map_value_source_type source_type;
    int cmd_argc;
    char **cmd_argv;
} map_config_t;

/*
 * Instantiates and initializes a new map_config_t.
 */
map_config_t new_map_config();

size_t calc_iobufsize();

#endif // CONFIG_H