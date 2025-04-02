#ifndef FILES_H
#define FILES_H

#include <stdlib.h>

/*
 * Maps the file identified by filepath to memory using mmap.
 */
void* mmap_file(const char *filepath, size_t *content_length);

#endif