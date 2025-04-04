#include "config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

map_config_t new_map_config() {
    return (map_config_t){
        NULL,
        0,
        NULL,
        DEFAULT_SEPARATOR_VALUE,
        0,
        MAP_VALUE_SOURCE_UNSPECIFIED,
        0,
        NULL
    };
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
