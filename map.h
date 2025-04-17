#ifndef MAP_H
#define MAP_H

#include "config.h"

typedef struct map_value {
    union {
        /* stream holding the map value (if not in msource) */
        FILE *fsource;

        /* pointer to the map value in memory (if not in fsource) */
        const char *msource;
    };

    /* length of the map value (if not msource) */
    size_t mlen;

    /* tracks the last byte from msource written to the destination */
    size_t pos;

    /* the input item to map: needed when the map value references the input item */
    char *item;
} map_value_t;

void map_ctx_init(map_value_t *v);

/*
    Copies at most max_len bytes of src into dst.
    Any occurrences of config->replstr will be replaced with src->item
    if both are present.

    note: src must have been initialized using map_vload.
 */
size_t map_vread(char *dst, size_t max_len, const map_config_t *config, map_value_t *v);

/*
 * Returns 1 if the source v has been consumed fully.
 */
int map_veof(const map_config_t *config, const map_value_t *v);

/*
 * Returns 1 if the underlying source v stream reports an error. 
 * Returns 0 in all other cases.
 */
int map_verr(const map_config_t *config, const map_value_t *v);

/*
 * Closes any stream associated with v and resets the internal offset.
 */
void map_vclose(const map_config_t *config, map_value_t *v);

/*
 * Resets the value source so that the next read will start from the beginning
 * of the value.
 * 
 * If the value source is of type MAP_VALUE_SOURCE_CMD, this command
 * will pclose the underlying stream.
 * 
 * In the other cases, it will reset the read offset to 0 only.
 * 
 * You should use map_vclose if you do not plan to read from v again.
 */
void map_vreset(const map_config_t *config, map_value_t *v);

/*
 * Loads the value source as per the type specified in config.
 * Exits the program upon failure or validation issue.
 */
void map_vload(const map_config_t *config, map_value_t *v);

#endif // MAP_H
