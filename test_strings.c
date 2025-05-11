/*
 * map - a fast CLI for mapping and transforming input to output.
 *
 * Copyright (c) 2025, Alessandro Diaferia < alediaferia at gmail dot com >
 * 
 * File: test_strings.c
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "test_strings.h"
#include "strings.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_strreplall(void) {
    char src[] = "This is just a string";
    char replstr[] = "is";
    char v[] = "was";
    char expected[] = "Thwas was just a string";

    const char *strreplall_r = strreplall(src, strlen(src), replstr, v, NULL);

    assert(strcmp(strreplall_r, expected) == 0);

    free((void*)strreplall_r);
}

void test_strreplall_nooccurs(void) {
    char src[] = "This is a string without replstr occurrences";
    char replstr[] = "@}{";
    char v[] = "smth";

    const char *output = strreplall(src, strlen(src), replstr, v, NULL);
    assert(strcmp(output, src) == 0);

    free((void*)output);
}

void test_strings(void) {

    test_strreplall();
    test_strreplall_nooccurs();
}
