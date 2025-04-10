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

void test_mvload_cmdline_replstr(void) {
    map_config_t config;
    config.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
    config.replstr = "@@@";
    config.vstatic = "Hello @@@!";

    map_ctx_t ctx;
    ctx.item = "World";
    ctx.pos = 0;
    ctx.msource = NULL;

    mvload(&config, &ctx);
    
    assert(strcmp(ctx.msource, "Hello World!") == 0);
    free((void*)ctx.msource);
}

void test_mvclose_cmdline_replstr(void) {
    map_config_t config;
    config.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
    config.replstr = "@@@";
    config.vstatic = "Hello @@@!";

    map_ctx_t ctx;
    ctx.msource = calloc(42, sizeof(char));

    mvclose(&config, &ctx);
    assert(ctx.msource == NULL);
}

void test_map(void) {
    test__map_replcmdargs();
    test__map_replcmdargs_multi_occurs();

    test_mvload_cmdline_replstr();
    test_mvclose_cmdline_replstr();
}