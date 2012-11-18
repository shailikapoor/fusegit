

#include <stdio.h>
#include <string.h>



int is_substr(const char *str, const char *sub)
{
	return strstr(str, sub) ? 1 : 0;
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
	} while (strlen(dummy) == 1);
	strcpy(name, dummy+1);
	return 0;
}

// Returns 0 if last component is reached, otherwise returns 1
int get_next_component(const char *path, int hier, char *name)
{
	path += strlen(name);
	name += strlen(name);
	while (!(*path == '/' || *path == '\0')) {
		*(name++) = *(path++);
	}
	*name = *path;
	if (*path == '\0')
		return 0;
	*(++name) = '\0';
	return 1;
}

