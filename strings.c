#include "strings.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *strrepl(const char *src, const char *replstr, const char *v, int *pos) {
    const size_t replstr_len = strlen(replstr);
    if (replstr_len == 0) {
        if (pos) {
            *pos = -1;
        }
        return src;
    }

    const size_t vlen = strlen(v);
    if (vlen == 0) {
        if (pos) {
            *pos = -1;
        }

        return src;
    }

    char *t = strstr(src, replstr);
    if (t == NULL) {
        if (pos) {
            *pos = -1;
        }
        return src;
    }

    const size_t newsize = strlen(src) - replstr_len + vlen + 1;
    
    char *r = calloc(newsize, sizeof(char));
    memcpy(r, src, t - src); // copy src until the replstr index
    memcpy(r + (t - src), v, vlen); // copy v
    strcpy(r + (t - src + vlen), t + replstr_len); // copy the remainder of src

    if (pos) {
        *pos = t - src + vlen + 1;
    }

    return r;
}

const char *strreplall(const char *src, const char *replstr, const char *v) {
    size_t size = strlen(src);
    char *replaced = calloc(size + 1, sizeof(char));
    if (replaced == NULL) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    size_t replstrlen = strlen(replstr);
    size_t vlen = strlen(v);

    // storing a copy of src to act on
    memcpy(replaced, src, strlen(src));

    int replpos = -1;
    int offset = 0; // the offset on the original string (replaced)

    do {
        const char *r = strrepl(replaced + offset, replstr, v, &replpos);
        
        /*
            r contains the original string with 1 occurrence of replstr replaced with v
            replpos now points at the index right after the first *replaced* (so v) occurrence of replstr.
            we want to update replaced with the contents of r before the next iteration, so:
            1. resize replaced to hold r (original size - replstr size + size of v)
            2. copy r from offset to offset + replpos
            3. update offset to replpos + offset
        */

        if (replpos != -1) {
            size_t newsize = size - replstrlen + vlen;
            replaced = realloc(replaced, newsize);
            if (replaced == NULL) {
                perror("Unable to allocate memory");
                exit(EXIT_FAILURE);
            }
            memcpy(replaced + offset, r, replpos);

            offset += replpos;
            size = newsize;

            free((void*)r);
        }
    } while (replpos != -1);

    return replaced;
}
