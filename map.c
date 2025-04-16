#include "map.h"
#include "cmd.h"
#include "files.h"
#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

char** _map_replcmdargs(const char *replstr, const char *v, int argc, char *argv[]);
void _mvloadcmd(const map_config_t *config, map_value_t *v);

void map_ctx_init(map_value_t *v) {
    memset(v, 0, sizeof(map_value_t));
}

size_t map_vread(char *dst, size_t max_len, const map_config_t *config, map_value_t *v) {
    size_t len;
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            /* relying on fsource's internal offset - no need to update ours */
            return fread(dst, sizeof(char), max_len, v->fsource);
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
        case MAP_VALUE_SOURCE_FILE:
            len = strlcpy(dst, v->msource + v->pos, max_len);
            v->pos += len;
            return len;
        default:
            fprintf(stderr, "Error: map value unspecified\n");
            return 0;
    }
}

int map_veof(const map_config_t *config, const map_value_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            return feof(v->fsource);
        case MAP_VALUE_SOURCE_FILE:
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
        default:
            return v->pos >= v->mlen;
    }
}

int map_verr(const map_config_t *config, const map_value_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            return ferror(v->fsource);
        default:
            return 0;
    }
}

void map_vreset(const map_config_t *config, map_value_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            pclose(v->fsource);
            v->fsource = NULL;
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            if (config->replstr) {
                v->pos = 0;
                free((void*)v->msource);
                v->msource = NULL;
            }
        default:
            v->pos = 0;
            break;
    }
}

void map_vclose(const map_config_t *config, map_value_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_CMD:
            pclose(v->fsource);
            v->fsource = NULL;
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            v->pos = 0;
            if (config->replstr) {
                free((void*)v->msource);
            }
            v->msource = NULL;
            break;
        case MAP_VALUE_SOURCE_FILE:
            v->pos = 0;
            if (v->msource != NULL) {
                if (config->replstr) {
                    free((void*)v->msource);
                } else {
                    munmap((void*)(v->msource), v->mlen);
                }
                v->msource = NULL;
                v->mlen = 0;
            }
            break;
        default:
            break;
        /* no op for now */
    }
}

void _mvloadcmd(const map_config_t *config, map_value_t *v) {
    char **p_argv = config->cmd_argv;
    int argc = config->cmd_argc;
    if (config->replstr) {
        p_argv = _map_replcmdargs(config->replstr, v->item, config->cmd_argc, config->cmd_argv);
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
        p_argv[argc++] = v->item;
    }

    v->fsource = runcmd(argc, p_argv);
    if (v->fsource == NULL) {
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

void _map_vload_src_f(const map_config_t *config, map_value_t *v) {
    if (v->msource == NULL) {
        v->msource = mmap_file(config->vfpath, &(v->mlen));
        if (v->msource == NULL) {
            exit(EXIT_FAILURE);
        }

        if (config->replstr) {
            const char *mmapped = v->msource;
            v->msource = strreplall(v->msource, config->replstr, v->item);
            munmap((void*)mmapped, v->mlen);
            v->mlen = strlen(v->msource);
        }
    }
}

void map_vload(const map_config_t *config, map_value_t *v) {
    switch (config->source_type) {
        case MAP_VALUE_SOURCE_UNSPECIFIED:
            fprintf(stderr, "Error: map value unspecified\n");
            exit(EXIT_FAILURE);
        case MAP_VALUE_SOURCE_FILE:
            _map_vload_src_f(config, v);
            break;
        case MAP_VALUE_SOURCE_CMDLINE_ARG:
            if (v->msource == NULL) {
                if (config->replstr) {
                    v->msource = strreplall(config->vstatic, config->replstr, v->item);
                } else {
                    v->msource = config->vstatic;
                }
                v->mlen = strlen(v->msource);
            }
            break;
        case MAP_VALUE_SOURCE_CMD:
            if (v->fsource == NULL) {
                _mvloadcmd(config, v);
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