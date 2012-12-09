#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/types.h>
 
struct dirent *dptr;
int main(int argc, char **argv){

	if(argc < 3) 
	{
		printf("Not Enough Arguments. Try again\n");
		exit(1);
	}
	
	int result_code = rmdir(argv[1], argv[2]);
	
	if(result_code == 0){
		printf("%s has been renamed to %s.\n", argv[1], argv[2]);
	}
	else{
		printf("Renaming Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	
	return 0;

}