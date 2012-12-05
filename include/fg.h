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
#define	MAX_HARD_LINKS	100	// FIXIT : this can be fixed by storing the link
				// information in multiple file notes, instead 
				// of just one
#define MAX_FMT_SIZE 4096

enum fg_task {
	FG_MOUNT = 1,
	FG_BACKUP,
	FG_RESTORE,
};

enum fg_opts {
	FG_OPT_BACKUP = 1,
	FG_OPT_RESTORE,
	FG_OPT_MOUNT,
	FG_OPT_DEBUG,
	FG_OPT_INVALID,
};

void debug_print(char *fmt, ...);

int fg_fuse_main(int argc, char *argv[]);

#ifdef DEBUG_PRINT_ENABLED
#define DEBUG debug_print
#else
#define DEBUG(format, args...) ((void)0)
#endif
