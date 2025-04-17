CC = gcc
CFLAGS = -Wall -Wextra -O3 -pedantic -std=c17
TARGET = map

CMD_SRCS = main.c

# Source files and object files
SRCS = cmd.c files.c options.c map.c buffers.c strings.c
OBJS = $(SRCS:.c=.o) $(CMD_SRCS:.c=.o)

# Test source and object
TEST_SRC = $(wildcard test*.c)
TEST_OBJ = $(TEST_SRC:.c=.o) $(SRCS:.c=.o)
TEST_TARGET = tests

.PHONY: all clean test debug

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
	rm -f $(OBJS) $(TEST_OBJ) $(TARGET) $(TEST_TARGET)

optimized: CFLAGS = -Wall -Wextra -O3
optimized: TARGET = map_optimized
optimized: clean all