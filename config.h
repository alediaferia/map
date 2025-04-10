#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdio.h>

#define DEFAULT_SEPARATOR_VALUE '\n'

enum map_vsource {
    MAP_VALUE_SOURCE_UNSPECIFIED = -1,
    MAP_VALUE_SOURCE_CMDLINE_ARG = 0,
    MAP_VALUE_SOURCE_FILE,
    MAP_VALUE_SOURCE_CMD
};

typedef struct map_config {
    char *vstatic; // the value to map to
    char *vfpath; // the file path to the value to map to
    char separator;
    char concatenator;
    enum map_vsource source_type;
    int cmd_argc;
    char **cmd_argv;
    int stripinput_flag;
    char *replstr;
} map_config_t;

/*
 * Instantiates and initializes a new map_config_t.
 */
map_config_t new_map_config();


#endif // CONFIG_H