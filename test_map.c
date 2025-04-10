#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "map.h"

char** _map_replcmdargs(const char *replstr, const char *v, int argc, char *argv[]);
void test__map_replcmdargs(void) {
    int cmd_argc = 4;
    char *args[] = { "program", "arg1", "argtorepl", "arg3" };
    char replstr[] = "argtorepl";
    char item[] = "arg2";

    char **replargs = _map_replcmdargs(replstr, item, cmd_argc, args);
    assert(replargs);

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
    assert(strcmp(replargs[2], "complex__arg__") == 0);
}

void test_map(void) {
    test__map_replcmdargs();
    test__map_replcmdargs_multi_occurs();
}