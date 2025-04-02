#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"

void test_example(void) {
    // Test case example
    assert(1 == 1);
}

void test_bufsize(void) {
    assert(calc_stdio_buffer_size() > 1);
}

int main(void) {
    printf("Running tests...\n");
    
    // Add test cases here
    test_example();
    test_bufsize();
    
    printf("\x1b[32mAll tests passed!\x1b[0m\n");
    return 0;
}
