#ifndef CMD_H
#define CMD_H

#include <stdio.h>

/* 
 * Executes the given command and returns a stream to
 * the command standard output.
 * 
 * It's the caller's responsibility to pclose the stream.
 */
FILE* runcmd(int argc, char *argv[]);

#endif // CMD_H
