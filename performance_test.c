/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * File: performance_test.c
 * Description: Comprehensive performance tests for the map utility
 */

 #include <fcntl.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/resource.h>
 #include <sys/stat.h>
 #include <sys/time.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <time.h>
 #include <unistd.h>
 
 #include "map.h"
 
 // Performance test configuration
 typedef struct perf_test_config {
   const char *name;                // Test name
   size_t input_size_mb;            // Input size in MB
   size_t item_size_bytes;          // Size of each input item in bytes
   const char *separator;           // Input separator
   const char *concatenator;        // Output concatenator
   const char *replacement_pattern; // Pattern to replace (if applicable)
   double replacement_frequency;    // How often the pattern appears (0.0-1.0)
   enum map_vsource source_type;    // Type of mapping source
   const char *alternative_cmd;     // Alternative command for comparison
   int discard_input;               // Whether to discard input during mapping
 } perf_test_config_t;
 
 // Performance metrics collected during a test
 typedef struct perf_metrics {
   double wall_time;           // Total elapsed wall time in seconds
   double user_cpu_time;       // User CPU time in seconds
   double sys_cpu_time;        // System CPU time in seconds
   double cpu_utilization;     // CPU utilization percentage
   long memory_before;         // Memory usage before test (KB)
   long memory_after;          // Memory usage after test (KB)
   double throughput_mb_per_s; // Processing throughput in MB/s
   double items_per_s;         // Items processed per second
   long output_size_map;       // Size of the output from map in bytes
   long output_size_alt;       // Size of the output from alternative command in bytes
 } perf_metrics_t;
 
 // Test file paths
 #define INPUT_FILE "/tmp/map_perf_input.txt"
 #define VALUE_FILE "/tmp/map_perf_value.txt"
 #define OUTPUT_FILE "/tmp/map_perf_output.txt"
 #define ALT_OUTPUT_FILE "/tmp/map_perf_alt_output.txt"
 #define LOG_FILE "/tmp/map_perf_log.txt"
 
 // Default values
 #define DEFAULT_PATTERN "@REPLACE_ME@"
 #define DEFAULT_REPLACEMENT "replacement"
 
 // Forward declarations
 void create_test_input_file(const char *path, size_t size_mb, size_t item_size,
                             const char *separator, const char *pattern,
                             double pattern_frequency);
 void create_comparison_table(const char *test_name, perf_metrics_t metrics_map,
                              perf_metrics_t metrics_alt, const char *alt_cmd);
 void create_test_value_file(const char *path, size_t size_kb,
                             const char *pattern, double pattern_frequency);
 perf_metrics_t run_performance_test(perf_test_config_t *config);
 void print_metrics(const char *test_name, perf_metrics_t metrics);
 void cleanup_test_files();
 
 /*
  * Creates a test input file with the specified characteristics
  */
 void create_test_input_file(const char *path, size_t size_mb, size_t item_size,
                             const char *separator, const char *pattern,
                             double pattern_frequency) {
   FILE *file = fopen(path, "w");
   if (!file) {
     perror("Failed to create input test file");
     exit(EXIT_FAILURE);
   }
 
   // Calculate number of items needed to reach the target file size
   size_t separator_len = strlen(separator);
   size_t bytes_per_item = item_size + separator_len;
   size_t target_size = size_mb * 1024 * 1024;
   size_t num_items = target_size / bytes_per_item;
 
   // Create a template item that will be used for all items
   char *item_template = calloc(item_size + 1, sizeof(char));
   if (!item_template) {
     perror("Failed to allocate memory for item template");
     fclose(file);
     exit(EXIT_FAILURE);
   }
 
   // Fill the template with random data
   const char *alphabet =
       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   size_t alphabet_len = strlen(alphabet);
 
   for (size_t i = 0; i < item_size; i++) {
     item_template[i] = alphabet[rand() % alphabet_len];
   }
 
   // Calculate pattern insertion points if needed
   size_t pattern_len = strlen(pattern);
   size_t num_patterns = (size_t)(item_size * pattern_frequency / pattern_len);
 
   if (num_patterns > 0 && pattern_len > 0) {
     // We'll insert the pattern at regular intervals
     size_t interval = item_size / (num_patterns + 1);
     for (size_t i = 1; i <= num_patterns; i++) {
       size_t pos = i * interval;
       if (pos + pattern_len <= item_size) {
         memcpy(item_template + pos, pattern, pattern_len);
       }
     }
   }
 
   // Write items to the file
   for (size_t i = 0; i < num_items; i++) {
     fwrite(item_template, sizeof(char), item_size, file);
     fwrite(separator, sizeof(char), separator_len, file);
 
     // Flush periodically to avoid buffer issues with very large files
     if (i % 10000 == 0) {
       fflush(file);
     }
   }
 
   free(item_template);
   fclose(file);
 
   // Verify file size
   struct stat st;
   stat(path, &st);
   printf("Created input file: %s (%ld bytes, %.2f MB)\n", path, st.st_size,
          st.st_size / (1024.0 * 1024.0));
 }
 
 /*
  * Creates a value file to be used with --value-file option
  */
 void create_test_value_file(const char *path, size_t size_kb,
                             const char *pattern, double pattern_frequency) {
   FILE *file = fopen(path, "w");
   if (!file) {
     perror("Failed to create value test file");
     exit(EXIT_FAILURE);
   }
 
   size_t target_size = size_kb * 1024;
   size_t pattern_len = strlen(pattern);
   size_t written = 0;
 
   // Fill with random content and occasional pattern insertions
   const char *alphabet =
       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   size_t alphabet_len = strlen(alphabet);
 
   char buffer[1024];
   size_t buffer_size = sizeof(buffer) - 1; // Leave room for null terminator
 
   while (written < target_size) {
     // Fill buffer with random data
     for (size_t i = 0; i < buffer_size; i++) {
       buffer[i] = alphabet[rand() % alphabet_len];
     }
     buffer[buffer_size] = '\0';
 
     // Insert pattern at random positions based on frequency
     size_t num_patterns =
         (size_t)(buffer_size * pattern_frequency / pattern_len);
     for (size_t i = 0; i < num_patterns; i++) {
       size_t pos = rand() % (buffer_size - pattern_len + 1);
       memcpy(buffer + pos, pattern, pattern_len);
     }
 
     // Write buffer to file
     size_t to_write = (written + buffer_size > target_size)
                           ? (target_size - written)
                           : buffer_size;
     fwrite(buffer, sizeof(char), to_write, file);
     written += to_write;
   }
 
   fclose(file);
 
   // Verify file size
   struct stat st;
   stat(path, &st);
   printf("Created value file: %s (%ld bytes, %.2f KB)\n", path, st.st_size,
          st.st_size / 1024.0);
 }
 
 /*
  * Run map test with the given configuration
  */
 perf_metrics_t run_map_test(perf_test_config_t *config, const char *output_file, 
                             const char *log_file) {
   perf_metrics_t metrics;
   struct rusage start_usage, end_usage;
   struct timeval start_time, end_time;
 
   printf("\n======== Running test: %s ========\n", config->name);
 
   // Calculate expected number of items
   size_t bytes_per_item = config->item_size_bytes + strlen(config->separator);
   size_t total_bytes = config->input_size_mb * 1024 * 1024;
   size_t expected_items = total_bytes / bytes_per_item;
 
   printf("Input size: %zu MB\n", config->input_size_mb);
   printf("Item size: %zu bytes\n", config->item_size_bytes);
   printf("Expected items: %zu\n", expected_items);
 
   // Create input file with specified parameters
   create_test_input_file(INPUT_FILE, config->input_size_mb,
                          config->item_size_bytes, config->separator,
                          config->replacement_pattern,
                          config->replacement_frequency);
 
   // Create value file if needed
   if (config->source_type == MAP_VALUE_SOURCE_FILE) {
     create_test_value_file(VALUE_FILE, 64, config->replacement_pattern,
                            config->replacement_frequency);
   }
 
   // Build command to run the map utility
   char cmd[1024] = {0};
 
   // Base command that redirects input and output
   sprintf(cmd, "cat %s | ./map ", INPUT_FILE);
 
   // Add separator and concatenator if specified
   if (strcmp(config->separator, "\n") != 0) {
     sprintf(cmd + strlen(cmd), "-s '%s' ", config->separator);
   }
 
   if (strcmp(config->concatenator, config->separator) != 0) {
     sprintf(cmd + strlen(cmd), "-c '%s' ", config->concatenator);
   }
 
   // Add replacement pattern if specified
   if (config->replacement_pattern && strlen(config->replacement_pattern) > 0) {
     sprintf(cmd + strlen(cmd), "-I '%s' ", config->replacement_pattern);
   }
 
   // Add discard input flag if specified
   if (config->discard_input) {
     strcat(cmd, "--discard-input ");
   }
 
   // Add source type parameter
   switch (config->source_type) {
   case MAP_VALUE_SOURCE_CMDLINE_ARG:
     strcat(cmd, "-v 'static_value' ");
     break;
   case MAP_VALUE_SOURCE_FILE:
     sprintf(cmd + strlen(cmd), "--value-file %s ", VALUE_FILE);
     break;
   case MAP_VALUE_SOURCE_CMD:
     strcat(cmd, "--value-cmd -- echo 'cmd_output' ");
     break;
   default:
     fprintf(stderr, "Invalid source type\n");
     exit(EXIT_FAILURE);
   }
 
   // Redirect output to file and add time measurements
   sprintf(cmd + strlen(cmd), "> %s 2> %s", output_file, log_file);
 
   printf("Executing: %s\n", cmd);
 
   // Get initial resource usage
   getrusage(RUSAGE_CHILDREN, &start_usage);
   gettimeofday(&start_time, NULL);
 
   // Run the command
   int ret = system(cmd);
   if (ret != 0) {
     fprintf(stderr, "Command execution failed with return code %d\n", ret);
     // Don't exit, we'll still report the metrics
   }
 
   // Get final resource usage
   gettimeofday(&end_time, NULL);
   getrusage(RUSAGE_CHILDREN, &end_usage);
 
   // Calculate metrics
   metrics.wall_time = (end_time.tv_sec - start_time.tv_sec) +
                       ((end_time.tv_usec - start_time.tv_usec) / 1000000.0);
 
   metrics.user_cpu_time =
       (end_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec) +
       ((end_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec) / 1000000.0);
 
   metrics.sys_cpu_time =
       (end_usage.ru_stime.tv_sec - start_usage.ru_stime.tv_sec) +
       ((end_usage.ru_stime.tv_usec - start_usage.ru_stime.tv_usec) / 1000000.0);
 
   metrics.cpu_utilization =
       ((metrics.user_cpu_time + metrics.sys_cpu_time) / metrics.wall_time) *
       100;
 
   metrics.memory_before = start_usage.ru_maxrss;
   metrics.memory_after = end_usage.ru_maxrss;
 
   struct stat st;
   if (stat(output_file, &st) == 0) {
     metrics.output_size_map = st.st_size;
     metrics.output_size_alt = st.st_size; // Will be overwritten for alternative command
   } else {
     metrics.output_size_map = 0;
     metrics.output_size_alt = 0;
   }
 
   // Calculate throughput
   metrics.throughput_mb_per_s = (config->input_size_mb) / metrics.wall_time;
   metrics.items_per_s = expected_items / metrics.wall_time;
 
   return metrics; 
 }
 
 /*
  * Run alternative command performance test
  */
 perf_metrics_t run_alt_test(perf_test_config_t *config, const char *output_file, 
                             const char *log_file) {
   if (!config->alternative_cmd || strlen(config->alternative_cmd) == 0) {
     // No alternative command specified, return empty metrics
     perf_metrics_t empty_metrics = {0};
     return empty_metrics;
   }
 
   perf_metrics_t metrics;
   struct rusage start_usage, end_usage;
   struct timeval start_time, end_time;
 
   printf("\n======== Running alternative test: %s ========\n", config->name);
 
   // Calculate expected number of items (same as map test)
   size_t bytes_per_item = config->item_size_bytes + strlen(config->separator);
   size_t total_bytes = config->input_size_mb * 1024 * 1024;
   size_t expected_items = total_bytes / bytes_per_item;
 
   // Build alternative command with proper substitutions based on config
   char cmd[1024] = {0};
   sprintf(cmd, "cat %s | %s", INPUT_FILE, config->alternative_cmd);
 
   // Redirect output and errors
   sprintf(cmd + strlen(cmd), " > %s 2> %s", output_file, log_file);
 
   printf("Executing alternative: %s\n", cmd);
 
   // Get initial resource usage
   getrusage(RUSAGE_CHILDREN, &start_usage);
   gettimeofday(&start_time, NULL);
 
   // Run the command
   int ret = system(cmd);
   if (ret != 0) {
     fprintf(stderr, "Alternative command execution failed with return code %d\n", ret);
     // Don't exit, we'll still report the metrics
   }
 
   // Get final resource usage
   gettimeofday(&end_time, NULL);
   getrusage(RUSAGE_CHILDREN, &end_usage);
 
   // Calculate metrics
   metrics.wall_time = (end_time.tv_sec - start_time.tv_sec) +
                       ((end_time.tv_usec - start_time.tv_usec) / 1000000.0);
 
   metrics.user_cpu_time =
       (end_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec) +
       ((end_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec) / 1000000.0);
 
   metrics.sys_cpu_time =
       (end_usage.ru_stime.tv_sec - start_usage.ru_stime.tv_sec) +
       ((end_usage.ru_stime.tv_usec - start_usage.ru_stime.tv_usec) / 1000000.0);
 
   metrics.cpu_utilization =
       ((metrics.user_cpu_time + metrics.sys_cpu_time) / metrics.wall_time) *
       100;
 
   metrics.memory_before = start_usage.ru_maxrss;
   metrics.memory_after = end_usage.ru_maxrss;
 
   struct stat st;
   if (stat(output_file, &st) == 0) {
     metrics.output_size_alt = st.st_size;
   } else {
     metrics.output_size_alt = 0;
   }
 
   // Calculate throughput
   metrics.throughput_mb_per_s = (config->input_size_mb) / metrics.wall_time;
   metrics.items_per_s = expected_items / metrics.wall_time;
 
   return metrics;
 }
 
 /*
  * Run both map and alternative command tests and return map metrics
  */
 perf_metrics_t run_performance_test(perf_test_config_t *config) {
   perf_metrics_t metrics_map = run_map_test(config, OUTPUT_FILE, LOG_FILE);
   perf_metrics_t metrics_alt = run_alt_test(config, ALT_OUTPUT_FILE, LOG_FILE);
   create_comparison_table(config->name, metrics_map, metrics_alt, config->alternative_cmd);
   return metrics_map;
 }
 
 /*
  * Print detailed performance metrics
  */
 void print_metrics(const char *test_name, perf_metrics_t metrics) {
   printf("\n======== Results for %s ========\n", test_name);
   printf("Wall time: %.6f seconds\n", metrics.wall_time);
   printf("User CPU time: %.6f seconds\n", metrics.user_cpu_time);
   printf("System CPU time: %.6f seconds\n", metrics.sys_cpu_time);
   printf("Total CPU time: %.6f seconds\n",
          metrics.user_cpu_time + metrics.sys_cpu_time);
   printf("CPU utilization: %.2f%%\n", metrics.cpu_utilization);
   printf("Memory before: %ld KB\n", metrics.memory_before);
   printf("Memory after: %ld KB\n", metrics.memory_after);
   printf("Memory increase: %ld KB\n",
          metrics.memory_after - metrics.memory_before);
   printf("Processing speed: %.2f MB/s\n", metrics.throughput_mb_per_s);
   printf("Items processed per second: %.2f\n", metrics.items_per_s);
   printf("Output size: %ld bytes (%.2f MB)\n", metrics.output_size_map,
          metrics.output_size_map / (1024.0 * 1024.0));
   printf("===========================================\n");
 }
 
 /*
  * Create a comparison table between map and alternative command
  */
 void create_comparison_table(const char *test_name, perf_metrics_t metrics_map,
                              perf_metrics_t metrics_alt, const char *alt_cmd) {
   if (!alt_cmd || strlen(alt_cmd) == 0) {
     return;  // No alternative command to compare with
   }
 
   printf("\n======== Comparison for %s ========\n", test_name);
   printf("%-25s | %-15s | %-15s | %-15s\n", "Metric", "map", alt_cmd, "Difference");
   printf("------------------------------------------------------------------------------\n");
 
   // Calculate differences and ratios
   double time_diff = metrics_map.wall_time - metrics_alt.wall_time;
   double time_ratio = metrics_map.wall_time / metrics_alt.wall_time;
 
   double cpu_diff = (metrics_map.user_cpu_time + metrics_map.sys_cpu_time) -
                     (metrics_alt.user_cpu_time + metrics_alt.sys_cpu_time);
   
   double memory_diff = (metrics_map.memory_after - metrics_map.memory_before) -
                        (metrics_alt.memory_after - metrics_alt.memory_before);
   
   double throughput_diff = metrics_map.throughput_mb_per_s - metrics_alt.throughput_mb_per_s;
   double throughput_ratio = metrics_map.throughput_mb_per_s / metrics_alt.throughput_mb_per_s;
 
   // Print comparison table
   printf("%-25s | %-15.6f | %-15.6f | %-+.2f (%.2fx)\n", 
          "Wall time (s)", metrics_map.wall_time, metrics_alt.wall_time, time_diff, time_ratio);
 
   printf("%-25s | %-15.6f | %-15.6f | %-+.2f\n", 
          "CPU time (s)", metrics_map.user_cpu_time + metrics_map.sys_cpu_time, 
          metrics_alt.user_cpu_time + metrics_alt.sys_cpu_time, cpu_diff);
 
   printf("%-25s | %-15.2f | %-15.2f | %-+.2f\n", 
          "CPU utilization (%)", metrics_map.cpu_utilization, metrics_alt.cpu_utilization,
          metrics_map.cpu_utilization - metrics_alt.cpu_utilization);
 
   printf("%-25s | %-15.2f | %-15.2f | %-+.2f (%.2fx)\n", 
          "Throughput (MB/s)", metrics_map.throughput_mb_per_s, metrics_alt.throughput_mb_per_s,
          throughput_diff, throughput_ratio);
   printf("==============================================================================\n");
 }
 
 /*
  * Clean up test files
  */
 void cleanup_test_files() {
   unlink(INPUT_FILE);
   unlink(VALUE_FILE);
   unlink(OUTPUT_FILE);
   unlink(ALT_OUTPUT_FILE);
   unlink(LOG_FILE);
 }
 
 /*
  * Comprehensive performance test suite
  */
 void run_performance_test_suite() {
   // Seed the random number generator
   srand(time(NULL));
 
   // Basic configuration for tests
   perf_test_config_t base_config = {.name = "Base test",
                                     .input_size_mb = 10,
                                     .item_size_bytes = 100,
                                     .separator = "\n",
                                     .concatenator = "\n",
                                     .replacement_pattern = "",
                                     .replacement_frequency = 0.1,
                                     .source_type = MAP_VALUE_SOURCE_CMDLINE_ARG,
                                     .alternative_cmd = "",
                                     .discard_input = 0};
 
   // Test 1: Basic static value mapping (low volume)
   perf_test_config_t test1 = base_config;
   test1.name = "Static value mapping (small)";
   test1.input_size_mb = 1;
   test1.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   test1.alternative_cmd = "awk '{print \"static_value\"}'";
   perf_metrics_t metrics1 = run_performance_test(&test1);
   print_metrics(test1.name, metrics1);
 
   // Test 2: Basic static value mapping (medium volume)
   perf_test_config_t test2 = base_config;
   test2.name = "Static value mapping (medium)";
   test2.input_size_mb = 10;
   test2.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   test2.alternative_cmd = "awk '{print \"static_value\"}'";
   perf_metrics_t metrics2 = run_performance_test(&test2);
   print_metrics(test2.name, metrics2);
 
   // Test 3: Basic static value mapping (high volume)
   perf_test_config_t test3 = base_config;
   test3.name = "Static value mapping (large)";
   test3.input_size_mb = 50;
   test3.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   test3.alternative_cmd = "awk '{print \"static_value\"}'";
   perf_metrics_t metrics3 = run_performance_test(&test3);
   print_metrics(test3.name, metrics3);
 
   // Test 4: File-based value mapping
   perf_test_config_t test4 = base_config;
   test4.name = "File-based value mapping";
   test4.input_size_mb = 10;
   test4.source_type = MAP_VALUE_SOURCE_FILE;
   test4.alternative_cmd = "while IFS= read -r line; do sed \"s/@REPLACE_ME@/$line/g\" " VALUE_FILE;
   test4.replacement_pattern = DEFAULT_PATTERN;
   perf_metrics_t metrics4 = run_performance_test(&test4);
   print_metrics(test4.name, metrics4);
 
   // Test 5: Command-based value mapping
   perf_test_config_t test5 = base_config;
   test5.name = "Command-based value mapping";
   test5.input_size_mb = 10;
   test5.source_type = MAP_VALUE_SOURCE_CMD;
   test5.alternative_cmd = "xargs -I{} echo 'cmd_output' {}";
   perf_metrics_t metrics5 = run_performance_test(&test5);
   print_metrics(test5.name, metrics5);
 
   // Test 6: Pattern replacement (static value)
   perf_test_config_t test6 = base_config;
   test6.name = "Pattern replacement (static value)";
   test6.input_size_mb = 10;
   test6.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   test6.replacement_pattern = DEFAULT_PATTERN; 
   test6.alternative_cmd = "sed 's/" DEFAULT_PATTERN "/" DEFAULT_REPLACEMENT "/g'";
   test6.replacement_frequency = 0.2;
   perf_metrics_t metrics6 = run_performance_test(&test6);
   print_metrics(test6.name, metrics6);
 
   // Test 7: Pattern replacement (file value)
   perf_test_config_t test7 = base_config;
   test7.name = "Pattern replacement (file value)";
   test7.input_size_mb = 10;
   test7.source_type = MAP_VALUE_SOURCE_FILE;
   test7.replacement_pattern = DEFAULT_PATTERN;
   test7.alternative_cmd = "awk -v val=\"$(cat " VALUE_FILE " )\" '{replaced_val = val; gsub(/" DEFAULT_PATTERN "/, $0, replaced_val); print replaced_val }'";
   test7.replacement_frequency = 0.2;
   perf_metrics_t metrics7 = run_performance_test(&test7);
   print_metrics(test7.name, metrics7);
 
   // Test 8: Custom separators
   perf_test_config_t test8 = base_config;
   test8.name = "Custom separators";
   test8.input_size_mb = 10;
   test8.separator = ",";
   test8.concatenator = ";";
   test8.alternative_cmd = "tr ',' '\\n' | xargs -I{} echo 'static_value' | tr '\\n' ';'";
   test8.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   perf_metrics_t metrics8 = run_performance_test(&test8);
   print_metrics(test8.name, metrics8);
 
   // Test 9: Discard input
   perf_test_config_t test9 = base_config;
   test9.name = "Discard input";
   test9.input_size_mb = 10;
   test9.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   test9.alternative_cmd = "xargs -I{} echo 'static_value'";
   test9.discard_input = 1;
   perf_metrics_t metrics9 = run_performance_test(&test9);
   print_metrics(test9.name, metrics9);
 
   // Test 10: Large items
   perf_test_config_t test10 = base_config;
   test10.name = "Large items";
   test10.input_size_mb = 10;
   test10.item_size_bytes = 10000; // 10KB items
   test10.alternative_cmd = "xargs -I{} echo 'static_value'";
   test10.source_type = MAP_VALUE_SOURCE_CMDLINE_ARG;
   perf_metrics_t metrics10 = run_performance_test(&test10);
   print_metrics(test10.name, metrics10);
 
   // Clean up
   cleanup_test_files();
 
   // Print summary
   printf("\n======== Performance Test Summary ========\n");
   printf("Test 1: %.2f MB/s, %.2f items/s\n", metrics1.throughput_mb_per_s,
          metrics1.items_per_s);
   printf("Test 2: %.2f MB/s, %.2f items/s\n", metrics2.throughput_mb_per_s,
          metrics2.items_per_s);
   printf("Test 3: %.2f MB/s, %.2f items/s\n", metrics3.throughput_mb_per_s,
          metrics3.items_per_s);
   printf("Test 4: %.2f MB/s, %.2f items/s\n", metrics4.throughput_mb_per_s,
          metrics4.items_per_s);
   printf("Test 5: %.2f MB/s, %.2f items/s\n", metrics5.throughput_mb_per_s,
          metrics5.items_per_s);
   printf("Test 6: %.2f MB/s, %.2f items/s\n", metrics6.throughput_mb_per_s,
          metrics6.items_per_s);
   printf("Test 7: %.2f MB/s, %.2f items/s\n", metrics7.throughput_mb_per_s,
          metrics7.items_per_s);
   printf("Test 8: %.2f MB/s, %.2f items/s\n", metrics8.throughput_mb_per_s,
          metrics8.items_per_s);
   printf("Test 9: %.2f MB/s, %.2f items/s\n", metrics9.throughput_mb_per_s,
          metrics9.items_per_s);
   printf("Test 10: %.2f MB/s, %.2f items/s\n", metrics10.throughput_mb_per_s,
          metrics10.items_per_s);
 }
 
 int main(int argc, char *argv[]) {
   printf("==== Map Utility Performance Test Suite ====\n\n");
 
   run_performance_test_suite();
 
   return 0;
 }