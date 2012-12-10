#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <unistd.h>

char *mountpoint;
char dirname[20] = "fusegitTest";
char filename[20] = "fuseFileTest.txt";
FILE *fp;
FILE *istream;
char ch;
char buff[FILENAME_MAX];
FILE *source, *target;

/* Creates a new directory at the mount point */
void createDir()
{
	int r = mkdir("fusegitTest", 0777);
	if(r == 0){
		printf("Directory fusegitTest was created!\n");
	}
	else{
		printf("Creating new directory : Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	
}

/* Creates a file inside the newly created directory */
void createFile()
{
	if((istream = fopen ("fuseFileTest.txt", "r" ) ) != NULL){
		printf("Error Creating a File. File already exists\n");
	}
	else
	{
		fp=fopen("fuseFileTest.txt","w");
	
		if(!fp)
		{
			printf("Error Creating a File\n");
		}	
		fclose(fp);
	}
	
}

/* After the file is created, Write to the file */
void writeFile()
{
	fp=fopen("fuseFileTest.txt", "w");
	while((ch=getchar())!=EOF)
			putc(ch,fp);
			
	fclose(fp);
}

/* Read from the file*/
void readFile()
{
   fp=fopen("fuseFileTest.txt", "r");
	
   if( fp == NULL )
   {
      perror("Error while opening the file.\n");
   }
   printf("The contents of fuseFileTest.txt file are :- \n\n");
 
   while( ( ch = fgetc(fp) ) != EOF )
      printf("%c",ch);
 
   fclose(fp);
   
}
	
/* Rename a file or directory */
void rename()
{
	int r = rmdir("fusegitTest", "newdirTest");
	
	if(r == 0){
		printf("fusegitTest has been renamed to newdirTest.\n");
	}
	else{
		printf("Renaming Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
}

/* Get path of the current working directory*/
void getPath()
{
	char *path = NULL;	
	size_t size;	
	path = getcwd(path,size);	
	printf("This is current working directory: %s \n", path);
}

/* get attributes of the file*/
void statTest()
{
	struct stat fileStat;
	
	if(stat("fuseFileTest.txt",&fileStat) == -1)
	{ 
		perror("stat");
	}
 
 
	printf("Information for %s\n",argv[1]);
    printf("---------------------------\n");
    
	printf("File type:                \t");

           switch (fileStat.st_mode & S_IFMT) {
           case S_IFBLK:  printf("block device\n");            break;
           case S_IFCHR:  printf("character device\n");        break;
           case S_IFDIR:  printf("directory\n");               break;
           case S_IFIFO:  printf("FIFO/pipe\n");               break;
           case S_IFLNK:  printf("symlink\n");                 break;
           case S_IFREG:  printf("regular file\n");            break;
           case S_IFSOCK: printf("socket\n");                  break;
           default:       printf("unknown?\n");                break;
    }
	
	printf("I-node number:            \t%ld\n", (long) fileStat.st_ino);
	
	printf("Mode:                     \t%lo (octal)\n", (unsigned long) fileStat.st_mode);
	
	printf("Number of Hard Links:     \t%ld\n", (long)fileStat.st_nlink);
	
	printf("Ownership:                      UID=%ld   GID=%ld\n", (long)fileStat.st_uid, (long)fileStat.st_gid);
	
	printf("I/O Block Size: \t\t%ld\n", (long)fileStat.st_blksize);
	
	printf("Number of blocks allocated: \t%ld\n",(long)fileStat.st_blocks);
	
	printf("File Size: \t\t\t%ld bytes\n",(long)fileStat.st_size);
	
	printf("Last Status Modification: \t%s", ctime(&fileStat.st_ctime));
	
	printf("Last Data Modification: \t%s",ctime(&fileStat.st_mtime));
	
	printf("Last Access: \t\t\t%s",ctime(&fileStat.st_atime));
	 
    printf("File Permissions: \t\t");
    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n\n");
 
    printf("The file %s a symbolic link\n", (S_ISLNK(fileStat.st_mode)) ? "is" : "is not");
}

/* Read a directory*/
void readDir()
{
	DIR *dirp;
	
	if((dirp=opendir("fusegitTest")) == NULL){
		printf("Error!\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	else
	{
		while(dptr=readdir(dirp))
		{
			printf("%s\n",dptr->d_name);
		}
	}
}

/* Remove directory*/
void removeDir()
{
	int result_code = rmdir("fusegitTest");
		
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

/* Change permissions of file or directory*/
void changePerm()
{
	int rc = chmod(filename, 754) ;
	
	if(rc < 0){
		printf("Error no is : %d\n", errno);
		printf("Error description is : %s\n", strerror(errno));
	}
	else{
		printf("Permissions have been changed\n");
	}
}

/* Change working directory*/
void changeDir(char *path)
{
	if (chdir(path) == -1) 
	{  
        printf ("chdir failed - %s\n", strerror (errno));  
	}
	
	if (getcwd(buff, sizeof buff) != NULL) puts(buff);
	else puts("unknown");
}

/*Copy a file*/
void copyFile()
{
	source = fopen("fuseFileTest.txt", "r");
	target = fopen("targetFile.txt", "w");
	
	while( ( ch = fgetc(source) ) != EOF )
      fputc(ch, target);
 
   printf("File copied successfully.\n");
 
   fclose(source);
   fclose(target);
	
}

/*remove a file*/
void removeFile()
{
	int r = remove("targetFile.txt");
	if( status == 0 )
      printf("file deleted successfully.\n");
	else
	{
		printf("Removing file: Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) 
	{
		exit(1);
	}
	
	changedir(argv[1]);
	createdir();
	createFile();
	writeFile();
	readFile();
	copyFile();
	statTest();
	getPath();
	readDir();
	removeFile();
	readDir();
	
	return 0;
}