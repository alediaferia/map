#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "buffers.h"
#include "strings.h"
#include "map.h"

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

char** _map_replcmdargs(const char *replstr, const char *v, int argc, char *argv[]);
void test__map_replcmdargs(void) {
    int cmd_argc = 4;
    char *args[] = { "program", "arg1", "argtorepl", "arg3" };
    char replstr[] = "argtorepl";
    char item[] = "arg2";

    char **replargs = _map_replcmdargs(replstr, item, cmd_argc, args);
    assert(replargs);

    printf("Replacement result: %s\n", replargs[2]);
    assert(strcmp(replargs[2], item) == 0);

    free(replargs);
}

void test__map_replcmdargs_multi_occurs(void) {
    int cmd_argc = 4;
    char *args[] = { "program", "arg1", "complex{}arg{}", "arg3" };
    char replstr[] = "{}";
    char item[] = "__";

    char **replargs = _map_replcmdargs(replstr, item, cmd_argc, args);

    assert(replargs);
    printf("Replacement result: %s\n", replargs[2]);
    assert(strcmp(replargs[2], "complex__arg__") == 0);
}

int main(void) {
    printf("Running tests...\n");
    
    // Add test cases here
    test_example();
    test_bufsize();
    test_strrepl();
    test__map_replcmdargs();
    test__map_replcmdargs_multi_occurs();
    
    printf("\x1b[32mAll tests PASSED\x1b[0m\n");
    return 0;
}
