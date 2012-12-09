#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <unistd.h>
 
struct dirent *dptr;
int main(int argc, char **argv){

	char *path = NULL;
	
	size_t size;
	
	path = getcwd(path,size);
	
	printf("%s \n", path);
	
	return 0;
}
