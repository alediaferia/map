#ifndef MAP_H
#define MAP_H

#include "config.h"

typedef struct {
    FILE *fsource;
    const char *msource;
    size_t mlen;
    size_t pos;
} map_value_source_t;


map_value_source_t new_map_value_source();

/*
 * Copies at most max_len bytes of src into dst.
 * src must have been loaded using mvload.
 */
size_t mvread(char *dst, size_t max_len, const map_config_t *config, map_value_source_t *src);

/*
 * Returns 1 if the source v has been consumed fully.
 */
int mveof(const map_config_t *config, const map_value_source_t *v);

/*
 * Returns 1 if the underlying source v stream reports an error. 
 * Returns 0 in all other cases.
 */
int mverr(const map_config_t *config, const map_value_source_t *v);

/*
 * Closes any stream associated with v and resets the internal offset.
 */
void mvclose(const map_config_t *config, map_value_source_t *v);

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
void mvreset(const map_config_t *config, map_value_source_t *v);

/*
 * Loads the value source as per the type specified in config.
 * Exits the program upon failure or validation issue.
 */

void mvload(const map_config_t *config, map_value_source_t *source);

#endif // MAP_H
