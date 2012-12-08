#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h> 
#include <sys/types.h> 

int main(int argc, char **argv){

	if(argc < 2) 
	{
		exit(1);
	}
	
	int result_code = rmdir(argv[1]);
		
	if(result_code == 0){
		printf("Directory was removed!\n");	
	}
	else{
		printf("Removing  Directory : Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	return 0;

}