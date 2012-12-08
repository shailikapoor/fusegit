#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h> 
#include <sys/types.h> 

int main(int argc, char **argv){

	if(argc < 2) 
	{
		exit(1);
	}
	
	int result_code = mkdir(argv[1], 0777);
	
	
	if(result_code == 0){
		printf("Directory was created!\n");
	}
	else{
		printf("Creating new directory : Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	return 0;

}