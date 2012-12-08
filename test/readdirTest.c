#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/types.h>
 
struct dirent *dptr;
int main(int argc, char **argv){

	if(argc < 2) 
	{
		exit(1);
	}
	
	DIR *dirp;
	
	if((dirp=opendir(argv[1])) == NULL){
		printf("Error!\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	
	while(dptr=readdir(dirp))
	{
		printf("%s\n",dptr->d_name);
	}
	
	return 0;

}