#include "strings.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *strreplall(const char *src, const char *replstr, const char *v) {
    size_t replstrlen = strlen(replstr);
    if (replstrlen == 0) {
        return NULL;
    }

    size_t vlen = strlen(v);

    size_t occurs = 0;
    const char *tmp = src;
    while ((tmp = strstr(tmp, replstr)) != NULL) {
        occurs += 1;
        tmp += vlen;
    }

    size_t srclen = strlen(src);
    size_t newsize = srclen - (replstrlen * occurs) + (vlen * occurs) + 1;

    char *news = calloc(newsize, sizeof(char));
    if (news == NULL) {
        perror("Unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    tmp = src;
    char *s = NULL;
    char *dst = news;
    while ((s = strstr(tmp, replstr)) != NULL) {
        size_t len = s - tmp;
        memcpy(dst, tmp, len);
        dst += len;
        memcpy(dst, v, vlen);
        dst += vlen;
        tmp = s + replstrlen;
    }

    if ((size_t)(tmp - src) < srclen) {
        strcpy(dst, tmp);
    }
    
    return news;
}
