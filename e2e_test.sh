#!/bin/bash
# Basic test framework for the command line utility

# Compile the program
echo "Compiling the program..."
make

# Check if make was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

# Colors for test results
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test counter
TOTAL_TESTS=0
PASSED_TESTS=0

# Function to run a test
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_output="$3"
    local input="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Running test: $test_name... "
    
    # Run the command with the provided input
    actual_output=$(echo -e "$input" | eval "$command")
    expected_output=$(echo -e "$expected_output")
    
    # Compare the actual output with the expected output
    if [ "$actual_output" = "$expected_output" ]; then
        echo -e "${GREEN}PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAILED${NC}"
        echo "Expected: '$expected_output'"
        echo "Got:      '$actual_output'"
    fi
}

# Function to run an error test (expecting stderr output)
run_error_test() {
    local test_name="$1"
    local command="$2"
    local expected_error_pattern="$3"
    local input="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Running error test: $test_name... " 
    
    # Run the command with the provided input and capture stderr
    error_output=$(echo -e "$input" | eval "$command 2>&1 >/dev/null")
    
    # Check if the error output contains the expected pattern
    if echo "$error_output" | grep -q "$expected_error_pattern"; then
        echo -e "${GREEN}PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAILED${NC}"
        echo "Expected error pattern: '$expected_error_pattern'"
        echo "Got error:              '$error_output'"
    fi
}

# Create test files
echo "Creating test files..."
echo -n "test content" > test_file.txt
echo "multi-line\ntest\ncontent" > test_multiline.txt

# -----------------
# Basic Tests
# -----------------

# Test with static value (-v flag)
run_test "Basic static value" "./map --discard-input -v 'mapped'" "mapped\nmapped\n" "line1\nline2\n"

# Test with file value (--value-file flag)
run_test "Basic file value" "./map --discard-input --value-file test_file.txt" "test content\ntest content\n" "line1\nline2\n"

# Test with command value (--value-cmd flag and a simple echo command)
run_test "Basic command value" "./map --discard-input --value-cmd -- echo -n 'cmd output'" "cmd output\ncmd output\n" "line1\nline2\n"

# Test with command value passing input item through
run_test "Basic command value with item pass through" "./map --value-cmd -- echo -n 'cmd output'" "cmd output line1\ncmd output line2\n" "line1\nline2\n"

# Test with replacement string
run_test "Basic command value with replacement string" "./map -I {} --value-cmd -- echo -n 'This is {}'" "This is line1\nThis is line2\nThis is line3\n" "line1\nline2\nline3"
run_test "Static value with replacement string" "./map -I {} -v 'Hello {}'" "Hello World\nHello People\n" "World\nPeople"

# -----------------
# Custom Separator/Concatenator Tests
# -----------------

# Test with custom separator
run_test "Custom separator" "./map --discard-input -v 'mapped' -s ','" "mapped,mapped\n" "line1,line2\n"

# Test with custom separator and concatenator
run_test "Custom separator and concatenator" "./map --discard-input -v 'mapped' -s ',' -c ';'" "mapped;mapped\n" "line1,line2\n"

# -----------------
# Edge Case Tests
# -----------------

# Test with empty input
run_test "Empty input" "./map --discard-input -v 'mapped'" "" ""

# Test with multiple separators in a row with no content
run_test "Multiple separators" "./map --discard-input -v 'mapped'" "" "\n\n\n"

# Test with large input (simulation)
large_input=$(printf 'line\n%.0s' {1..100})
expected_large_output=$(printf 'mapped\n%.0s' {1..100})
run_test "Large input" "./map --discard-input -v 'mapped'" "$expected_large_output" "$large_input"

# -----------------
# Error Tests
# -----------------

# Test with missing value argument
run_error_test "Missing value argument" "./map" "One of -v, --value-file or --value-cmd must be explicitly specified" ""

# Test with non-existent file
run_error_test "Non-existent file" "./map --value-file nonexistent.txt" "Cannot open file" ""

# Test with invalid separator argument
run_error_test "Invalid separator" "./map -v 'mapped' -s 'ab'" "must be a single character" ""

# -----------------
# Test Results
# -----------------

# Clean up test files
rm -f test_file.txt test_multiline.txt

# Print test summary
echo -e "\n===================="
echo "Test Summary"
echo "===================="
echo -e "Tests run: $TOTAL_TESTS"
echo -e "Tests passed: $PASSED_TESTS"
echo -e "Tests failed: $((TOTAL_TESTS - PASSED_TESTS))"
echo -e "\nResult:"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}All tests PASSED${NC}"
    exit 0
else
    echo -e "${RED}Some tests FAILED${NC}"
    exit 1
fi