#include "test_strings.h"
#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_strreplall(void) {
    char src[] = "This is just a string";
    char replstr[] = "is";
    char v[] = "was";
    char expected[] = "Thwas was just a string";

    const char *strreplall_r = strreplall(src, replstr, v);

    assert(strcmp(strreplall_r, expected) == 0);

    free((void*)strreplall_r);
}

void test_strreplall_nooccurs(void) {
    char src[] = "This is a string without replstr occurrences";
    char replstr[] = "@}{";
    char v[] = "smth";

    const char *output = strreplall(src, replstr, v);
    assert(strcmp(output, src) == 0);

    free((void*)output);
}

void test_strings(void) {

    test_strreplall();
    test_strreplall_nooccurs();
}
