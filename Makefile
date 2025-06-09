CC = gcc
CFLAGS = -Wall -Wextra -O3 -pedantic -std=c17
DEFS_HEADER := defs.h
CFLAGS += -include $(DEFS_HEADER)
TARGET = map

CMD_SRCS = main.c

# Source files and object files
SRCS = cmd.c files.c options.c map.c buffers.c strings.c
OBJS = $(SRCS:.c=.o) $(CMD_SRCS:.c=.o)

# Test source and object
TEST_SRC = $(wildcard test*.c)
TEST_OBJ = $(TEST_SRC:.c=.o) $(SRCS:.c=.o)
TEST_TARGET = tests

# Performance test source and object files
PERF_TEST_SRC = performance_test.c
PERF_TEST_OBJ = $(PERF_TEST_SRC:.c=.o) $(SRCS:.c=.o)
PERF_TEST_TARGET = performance_test
PERF_TEST_SCRIPT = performance_test.sh
PERF_RESULTS_DIR = perf_test_results

.PHONY: all clean test debug performance-test performance-report performance-analyze

all: $(TARGET)

# Main target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Test target
test: CFLAGS += -g -DDEBUG -O0
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(TEST_OBJ) -o $(TEST_TARGET)

performance-test: $(TARGET) $(PERF_TEST_TARGET)
	./$(PERF_TEST_TARGET)

$(PERF_TEST_TARGET): $(PERF_TEST_OBJ)
	$(CC) $(PERF_TEST_OBJ) -o $(PERF_TEST_TARGET)

performance_test.o: $(PERF_TEST_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for object files
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Special case for main.o which doesn't have a .h file
main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build with symbols and debug info
debug: CFLAGS = -Wall -Wextra -O0 -g -DDEBUG
debug: clean all

clean:
	rm -f $(OBJS) $(TEST_OBJ) $(PERF_TEST_OBJ) $(TARGET) $(TEST_TARGET) $(PERF_TEST_TARGET)

optimized: CFLAGS = -Wall -Wextra -O3
optimized: TARGET = map_optimized
optimized: clean all
