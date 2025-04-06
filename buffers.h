#ifndef BUFFERS_H
#define BUFFERS_H

#include <unistd.h>
#include <stdio.h>

void bufflush(const char *buf, size_t len, FILE *dst);

size_t calc_iobufsize();

#endif // BUFFERS_H