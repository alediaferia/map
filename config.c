#include "config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

map_config_t new_map_config() {
    return (map_config_t){
        NULL,
        NULL,
        DEFAULT_SEPARATOR_VALUE,
        0,
        MAP_VALUE_SOURCE_UNSPECIFIED,
        0,
        NULL,
        0,
        NULL
    };
}


