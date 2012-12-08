#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h> 
#include <sys/types.h> 

int main(int argc, char **argv){

	if(argc < 2) 
	{
		exit(1);
	}
	
	int result_code = rmdir(argv[1]);
	
	
	if(result_code == 0){
		printf("Directory was created!\n");
	}
	else{
		printf("Directory was NOT created!\n");
		exit(1);
	}
	return 0;

}