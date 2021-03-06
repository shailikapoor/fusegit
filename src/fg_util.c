
#include <stdio.h>
#include <string.h>
#include "fg.h"

void debug_print(char *msg, ...)
{    
	char buf[MAX_FMT_SIZE+2];

	va_list argp;
        va_start(argp, msg);
	vsnprintf(buf, MAX_FMT_SIZE, msg, argp);
	va_end(argp);
	buf[strlen(buf)+1] = '\0';
	buf[strlen(buf)] = '\n';

	fprintf(stdout, "%s", buf);
}

int is_substr(const char *str, const char *sub)
{
	return strstr(str, sub) ? 1 : 0;
}

int get_parent_path(const char *path, char *parent)
{
	char *tmp;
	int old_len;
	int new_len;

	if (path)
		strcpy(parent, path);
	tmp = rindex(parent, '/');
	old_len = strlen(parent);

	*(tmp+1) = '\0';
	new_len = strlen(parent);

	if ((old_len == new_len) && (strcmp(parent, "/") == 0))
		return -1;
	if (old_len == new_len) {
		*tmp = '\0';
		tmp = rindex(parent, '/');
		*(tmp+1) = '\0';
	}

	return 0;
}

int get_last_component(const char *path, char *name)
{
	if (*path != '/')
		return -1;
	char *dummy = NULL;
	strcpy(name, path);

	do {
		if (dummy)
			dummy[0] = '\0';
		dummy = rindex(name, '/');
	} while (dummy && strlen(dummy) == 1);
	if (dummy)
		strcpy(name, dummy+1);
	else
		*name = '\0';
	return 0;
}

// Returns 0 if last component is reached, otherwise returns 1
int get_next_component(const char *path, int hier, char *name)
{
	char *ind;
	int len = strlen(name);
	int count = 0;

	strcpy(name, path);
	ind = index(name, '/');
	while (count < hier) {
		count++;
		ind++;
		ind = index(ind, '/');
		if (ind == NULL) {
			if (len == strlen(name))
				return 0;
			else
				return 1;
		}
	}
	if (*ind == '/')
		*(ind+1) = '\0';
	return 1;
}

// print file stats
void print_file_stats(const char *path, struct stat *stbuf)
{
	DEBUG("***PRINT STAT : %s", path);
	DEBUG("\tdev=%x", (unsigned int)stbuf->st_dev);
	DEBUG("\tino=%x", (unsigned int)stbuf->st_ino);
	DEBUG("\tmode=%x", (unsigned int)stbuf->st_mode);
	DEBUG("\tnlink=%x", (unsigned int)stbuf->st_nlink);
	DEBUG("\tuid=%x", (unsigned int)stbuf->st_uid);
	DEBUG("\tgid=%x", (unsigned int)stbuf->st_gid);
	DEBUG("\tsize=%x", (unsigned int)stbuf->st_size);
	DEBUG("\tmtime=%x", (unsigned int)stbuf->st_mtime);
	DEBUG("\tctime=%x", (unsigned int)stbuf->st_ctime);
	DEBUG("***PRINT STAT OVER\n");
}
