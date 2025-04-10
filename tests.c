#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "buffers.h"
#include "strings.h"

#include "test_map.h"

void test_example(void) {
    // Test case example
    assert(1 == 1);
}

void test_bufsize(void) {
    assert(calc_iobufsize() > 1);
}

void test_strrepl(void) {
    const char src[] = "Hello my world";
    const char replstr[] = "my";
    int p = 0;
    const char v[] = "your";

    const char *replaced = strrepl(src, replstr, v, &p);

    assert(strcmp("Hello your world", replaced) == 0);
    assert(p == 11);

    free((void*)replaced);
}

int main(void) {
    printf("Running tests...\n");
    
    // Add test cases here
    test_example();
    test_bufsize();
    test_strrepl();

    test_map();
    
    printf("\x1b[32mAll tests PASSED\x1b[0m\n");
    return 0;
}
