#include "map.h"
#include "cmd.h"
#include "files.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

map_value_source_t new_map_value_source() {
    return (map_value_source_t){
        NULL,
        NULL,
        0,
        0
    };
}

size_t mvread(char *dst, size_t max_len, const map_config_t *config, map_value_source_t *src) {
    size_t len;
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            /* relying on fsource's internal offset - no need to update ours */
            return fread(dst, sizeof(char), max_len, src->fsource);
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
        case MAP_VALUE_SOURCE_FILE:
            len = strlcpy(dst, src->msource + src->pos, max_len);
            src->pos += len;
            return len;
        default:
            fprintf(stderr, "Error: map value unspecified\n");
            return 0;
    }
}

int mveof(const map_config_t *config, const map_value_source_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            return feof(v->fsource);
        case MAP_VALUE_SOURCE_FILE:
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
        default:
            return v->pos >= v->mlen;
    }
}

int mverr(const map_config_t *config, const map_value_source_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            return ferror(v->fsource);
        default:
            return 0;
    }
}

void mvreset(const map_config_t *config, map_value_source_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            pclose(v->fsource);
            v->fsource = NULL;
            break;
        default:
            v->pos = 0;
            break;
    }
}

void mvclose(const map_config_t *config, map_value_source_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            pclose(v->fsource);
            v->fsource = NULL;
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            v->pos = 0;
            break;
        case MAP_VALUE_SOURCE_FILE:
            v->pos = 0;
            if (v->msource != NULL) {
                munmap((void*)(v->msource), v->mlen);
                v->msource = NULL;
                v->mlen = 0;
            }
            break;
        default:
            break;
        /* no op for now */
    }
}

void mvload(const map_config_t *config, map_value_source_t *source) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_UNSPECIFIED:
            fprintf(stderr, "Error: map value unspecified\n");
            exit(EXIT_FAILURE);
        case MAP_VALUE_SOURCE_FILE:
            if (source->msource == NULL) {
                source->msource = mmap_file(config->vfpath, &(source->mlen));
                if (source->msource == NULL) {
                    exit(EXIT_FAILURE);
                }
            }
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            if (source->msource == NULL) {
                source->msource = config->vstatic;
                source->mlen = strlen(source->msource);
            }
            break;
        case MAP_VALUE_SOURCE_CMD:
            if (source->fsource == NULL) {
                source->fsource = runcmd(config->cmd_argc, config->cmd_argv);
                if (source->fsource == NULL) {
                    exit(EXIT_FAILURE);
                }
            }
            break;
    }
}