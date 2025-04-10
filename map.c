#include "map.h"
#include "cmd.h"
#include "files.h"
#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

char** _map_replcmdargs(const char *replstr, const char *v, int argc, char *argv[]);

map_ctx_t new_map_ctx() {
    return (map_ctx_t){
        NULL,
        NULL,
        0,
        0,
        NULL
    };
}

size_t mvread(char *dst, size_t max_len, const map_config_t *config, map_ctx_t *src) {
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

int mveof(const map_config_t *config, const map_ctx_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            return feof(v->fsource);
        case MAP_VALUE_SOURCE_FILE:
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
        default:
            return v->pos >= v->mlen;
    }
}

int mverr(const map_config_t *config, const map_ctx_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            return ferror(v->fsource);
        default:
            return 0;
    }
}

void mvreset(const map_config_t *config, map_ctx_t *v) {
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

void mvclose(const map_config_t *config, map_ctx_t *v) {
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

void _mvloadcmd(const map_config_t *config, map_ctx_t *source) {
    char **p_argv = config->cmd_argv;
    int argc = config->cmd_argc;
    if (config->replstr) {
        p_argv = _map_replcmdargs(config->replstr, source->item, config->cmd_argc, config->cmd_argv);
    } else if (config->stripinput_flag == 0) {
        /*
            If we are not stripping the input item,
            then we will be passing the input item as an additional
            command argument: therefore, we need to extend the current argv
            vector to hold one more arg.
        */

        p_argv = calloc((config->cmd_argc) + 2, sizeof(char*));
        if (p_argv == NULL) {
            perror("Unable to allocate memory");
            exit(EXIT_FAILURE);
        }
        memcpy(p_argv, config->cmd_argv, config->cmd_argc * sizeof(char*));
        p_argv[argc++] = source->item;
    }

    source->fsource = runcmd(argc, p_argv);
    if (source->fsource == NULL) {
        exit(EXIT_FAILURE);
    }

    if (config->replstr) {
        for (int i = 0; i < argc; i++) {
            free(p_argv[i]);
        }
        free(p_argv);
    } else if (config->stripinput_flag == 0) {
        free(p_argv);
    }
}

void mvload(const map_config_t *config, map_ctx_t *source) {
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
                _mvloadcmd(config, source);
            }
            break;
    }
}

/*
    Replaces the command line args with replstr if set and if applicable.
*/
char** _map_replcmdargs(const char *replstr, const char *v, int argc, char *argv[]) {
    char **dst = calloc(argc, sizeof(char*));
    if (!dst) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }
    memcpy(dst, argv, argc * sizeof(char *));

    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        char *arg = calloc(len + 1, sizeof(char));
        if (arg == NULL) {
            perror("Unable to allocate memory for arg");
            exit(EXIT_FAILURE);
        }
        strncpy(arg, argv[i], len);
        if (i > 0) { /* do not replace the command */
            arg = (char*)strreplall(arg, replstr, v);
        }
        dst[i] = arg;
    }

    return dst;
}