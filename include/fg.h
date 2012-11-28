#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#define DEBUG_PRINT_ENABLED 1
#define	PATH_MAX_LENGTH	1024
#define MAX_FMT_SIZE 4096

void debug_print(char *fmt, ...);

#ifdef DEBUG_PRINT_ENABLED
#define DEBUG debug_print
#else
#define DEBUG(format, args...) ((void)0)
#endif
