

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

