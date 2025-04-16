#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "buffers.h"
#include "strings.h"

#include "test_map.h"
#include "test_strings.h"

void test_example(void) {
    // Test case example
    assert(1 == 1);
}

void test_bufsize(void) {
    assert(calc_iobufsize() > 1);
}

int main(void) {
    printf("Running tests...\n");
    
    // Add test cases here
    test_example();
    test_bufsize();

    test_map();
    test_strings();
    
    printf("\x1b[32mAll tests PASSED\x1b[0m\n");
    return 0;
}
