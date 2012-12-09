#include<stdio.h>


int main(int argc, char **argv){
	
	FILE *fp;
	FILE *istream;
	char ch;
	
	if(argc < 2) 
	{
		exit(1);
	}
	
	if((istream = fopen (argv[1], "r" ) ) != NULL){
		printf("Error Creating a File. File already exists\n");
	}
	else
	{
		fp=fopen(argv[1],"w");
	
		if(!fp)
		{
			printf("Error Creating a File\n");
			exit(1);
		}
	
	
		printf("\nEnter data to be stored in to the file:");
	
		while((ch=getchar())!=EOF)
			putc(ch,fp);
	
		fclose(fp);
	}
	return 0;
}