#ifndef MAP_H
#define MAP_H

#include "config.h"

typedef struct {
    /* stream holding the map value (if not in msource) */
    FILE *fsource;

    /* pointer to the map value in memory (if not in fsource) */
    const char *msource;

    /* length of the map value (if not msource) */
    size_t mlen;

    /* tracks the last byte from msource written to the destination */
    size_t pos;

    /* the item to map */
    char *item;
} map_ctx_t;


map_ctx_t new_map_ctx();

/*
    Copies at most max_len bytes of src into dst.
    Any occurrences of config->replstr will be replaced with src->item
    if both are present.

    note: src must have been initialized using mvload.
 */
size_t mvread(char *dst, size_t max_len, const map_config_t *config, map_ctx_t *src);

/*
 * Returns 1 if the source v has been consumed fully.
 */
int mveof(const map_config_t *config, const map_ctx_t *v);

/*
 * Returns 1 if the underlying source v stream reports an error. 
 * Returns 0 in all other cases.
 */
int mverr(const map_config_t *config, const map_ctx_t *v);

/*
 * Closes any stream associated with v and resets the internal offset.
 */
void mvclose(const map_config_t *config, map_ctx_t *v);

/*
 * Resets the value source so that the next read will start from the beginning
 * of the value.
 * 
 * If the value source is of type MAP_VALUE_SOURCE_CMD, this command
 * will pclose the underlying stream.
 * 
 * In the other cases, it will reset the read offset to 0 only.
 * 
 * You should use mvclose if you do not plan to read from v again.
 */
void mvreset(const map_config_t *config, map_ctx_t *v);

/*
 * Loads the value source as per the type specified in config.
 * Exits the program upon failure or validation issue.
 */

void mvload(const map_config_t *config, map_ctx_t *source);

#endif // MAP_H
