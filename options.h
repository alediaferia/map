#ifndef OPTIONS_H
#define OPTIONS_H

#include "map.h"

void load_config_from_options(
    map_config_t *map_config,
    int *argc,
    char **argv[]
);

void print_usage(char *argv[]);

#endif // OPTIONS_H
