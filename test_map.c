#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>

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

    map_value_t ctx;
    ctx.item = "World";
    ctx.pos = 0;
    ctx.msource = NULL;

    map_vload(&config, &ctx);
    
    assert(strcmp(ctx.msource, "Hello World!") == 0);
    free((void*)ctx.msource);
}

void test_mvclose_cmdline_replstr(void) {
    map_config_t config;
    config.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
    config.replstr = "@@@";
    config.vstatic = "Hello @@@!";

    map_value_t ctx;
    ctx.msource = calloc(42, sizeof(char));

    map_vclose(&config, &ctx);
    assert(ctx.msource == NULL);
}

int _create_test_file(size_t size_mb, char *template, const char *pattern) {
    int fd = mkstemp(template);
    if (fd == -1) {
        perror("Failed to create temporary file");
        return -1;
    }
    
    if (fchmod(fd, S_IRUSR | S_IWUSR) == -1) {
        perror("Failed to change file permissions");
        close(fd);
        return -1;
    }
    
    size_t pattern_length = strlen(pattern);
    size_t iterations = (size_mb * 1024 * 1024) / pattern_length;
    
    FILE *file = fdopen(fd, "w");
    if (!file) {
        perror("Failed to open file stream");
        close(fd);
        return -1;
    }
    
    for (size_t i = 0; i < iterations; i++) {
        if (fputs(pattern, file) == EOF) {
            perror("Failed to write to temp file");
            fclose(file);
            return -1;
        }
    }
    
    fflush(file);
    
    assert(fd != -1);

    printf("Created temporary test file %s (%zu MB)\n", template, size_mb);
    
    return fd;
}

// Helper function to count occurrences of a substring within a length limit
size_t _count_occurrences(const char *str, const char *substr, size_t max_length) {
    size_t count = 0;
    const char *tmp = str;
    const char *end = str + max_length;
    size_t substr_len = strlen(substr);
    
    while (tmp < end && (tmp = strstr(tmp, substr)) != NULL) {
        count++;
        tmp += substr_len;
    }
    
    return count;
}

void test_perf_mvload_bigfile_replstr(void) {
    printf("====== Begin perf. test - map_vload bigfile replstr ======\n");
    map_config_t config;
    config.source_type = MAP_VALUE_SOURCE_FILE;

    const char replstr[] = "@@@@";
    const char pattern[] = "String pattern with substring @@@@ to replace";
    
    config.replstr = replstr;

    char ftemplate[] = "/tmp/tmp-test_mvload_bigfile_replstr-XXXXXX";
    int tmpfd = _create_test_file(10, ftemplate, pattern);
    if (tmpfd == -1) {
        perror("Unable to create temp file");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(tmpfd, &sb) == -1) {
        perror("Failed to get file size");
        close(tmpfd);
        exit(EXIT_FAILURE);
    }

    config.vfpath = ftemplate;
    
    map_value_t ctx;
    ctx.item = "Map Rocks!";
    ctx.msource = NULL;
    ctx.mlen = 0;

    size_t pattern_length = strlen(pattern);
    size_t occurrences_per_pattern = 1;
    size_t total_patterns = sb.st_size / pattern_length;
    size_t expected_occurrences = total_patterns * occurrences_per_pattern;

    printf("Expected occurrences of \"%s\": %zu\n", replstr, expected_occurrences);

    struct rusage start_usage, end_usage;
    getrusage(RUSAGE_SELF, &start_usage);
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    map_vload(&config, &ctx);

    gettimeofday(&end_time, NULL);
    getrusage(RUSAGE_SELF, &end_usage);
    
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                        ((end_time.tv_usec - start_time.tv_usec) / 1000000.0);
    
    double user_time = (end_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec) +
                      ((end_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec) / 1000000.0);
    double sys_time = (end_usage.ru_stime.tv_sec - start_usage.ru_stime.tv_sec) +
                     ((end_usage.ru_stime.tv_usec - start_usage.ru_stime.tv_usec) / 1000000.0);

    assert(ctx.msource != NULL);
    printf("File mapped & replaced successfully, size: %lld bytes (%.2f MB)\n", 
        sb.st_size, sb.st_size / (1024.0 * 1024.0));

    // Sample first 1MB to verify actual occurrences match expected
    size_t sample_size = 1024 * 1024;
    if (sample_size > sb.st_size) sample_size = sb.st_size;

    size_t replaced_count = _count_occurrences(ctx.msource, ctx.item, sample_size);
    size_t remaining_count = _count_occurrences(ctx.msource, replstr, sample_size);

    printf("\nReplacement Results:\n");
    printf("Original string occurrences remaining: %zu (should be 0)\n", remaining_count);
    printf("Replacement string occurrences: %zu (should be > 0)\n", 
           replaced_count);

    assert(remaining_count == 0);
    assert(replaced_count > 0);

    printf("\nPerformance Metrics:\n");
    printf("Wall time: %.6f seconds\n", elapsed_time);
    printf("User CPU time: %.6f seconds\n", user_time);
    printf("System CPU time: %.6f seconds\n", sys_time);
    printf("Total CPU time: %.6f seconds\n", user_time + sys_time);
    printf("CPU utilization: %.2f%%\n", ((user_time + sys_time) / elapsed_time) * 100);
    
    double mb_per_second = (sb.st_size / (1024.0 * 1024.0)) / elapsed_time;
    printf("Processing speed: %.2f MB/s\n", mb_per_second);
    printf("Replacements per second: %.2f\n", expected_occurrences / elapsed_time);
    
    long memory_before = start_usage.ru_maxrss;
    long memory_after = end_usage.ru_maxrss;
    printf("Memory usage before: %ld KB\n", memory_before);
    printf("Memory usage after: %ld KB\n", memory_after);
    printf("Memory increase: %ld KB\n", memory_after - memory_before);
    
    double bytes_per_replacement = ((memory_after - memory_before) * 1024.0) / expected_occurrences;
    printf("Memory per replacement: %.2f bytes\n", bytes_per_replacement);
            
    close(tmpfd);
    unlink(ftemplate);
    map_vclose(&config, &ctx);

    printf("====== End perf. test - map_vload bigfile replstr ======\n");
}

void test_map(void) {
    test__map_replcmdargs();
    test__map_replcmdargs_multi_occurs();

    test_mvload_cmdline_replstr();
    test_mvclose_cmdline_replstr();
    test_perf_mvload_bigfile_replstr();
}