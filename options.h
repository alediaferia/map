#ifndef OPTIONS_H
#define OPTIONS_H

#include "map.h"

void map_config_load_from_args(
    map_config_t *map_config,
    int *argc,
    char **argv[]
);

void print_usage(char *argv[]);

#endif // OPTIONS_H
