#include "strings.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    int replpos = -1;
    const char *cur = src;
    const char *replaced;

    int j = 0;
    do {
        replaced = strrepl(cur, replstr, v, &replpos);
        if (replpos != -1) {
            if (j > 0) {
                free((char*)cur);
            }
            cur = replaced;
        }

        j++;
    } while (replpos != -1);

    return replaced;
}
