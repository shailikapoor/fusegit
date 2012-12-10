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

/* Change working directory*/
void changeDir(char *path)
{
	printf("***START : change current working directory\n");
	char buff[4096];
	if (chdir(path) == -1) {
		printf ("chdir failed - %s\n", strerror (errno));  
	}
	
	if (getcwd(buff, sizeof buff) != NULL)
		printf("current working directory: %s\n", buff);
	else
		printf("unknown");
	printf("***OVER***\n");
}

/* Get path of the current working directory*/
void getPath()
{
	printf("***START : get current working directory\n");
	char path[1024];	
	size_t size = 1024;	
	getcwd(path, size);	
	printf("This is current working directory: %s \n", path);
	printf("***OVER***\n");
}

/* Creates a new directory at the mount point */
void createDir(const char *dir_name)
{
	printf("***START : create directory\n");
	int r;
	if ((r = mkdir(dir_name, 0777)) < 0) {
		printf("Creating new directory : Failed\n");
		printf ("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	printf("Directory fusegitTest was created!\n");
	printf("***OVER***\n");
}

/* Creates a file inside the newly created directory */
void createFile(const char *file_path)
{
	printf("***START : create file\n");

	FILE *f;

	if ((f = fopen(file_path, "r")) != NULL) {
		printf("Error!!! File already exists\n");
	} else {
		f = fopen(file_path,"w");
	
		if(!f) {
			printf("Error Creating a File\n");
		} else {
			printf("File creation is successful\n");
		}
		fclose(f);
	}
	printf("***OVER***\n");
}

/* After the file is created, Write to the file */
void writeFile(const char *file_path, const char *data)
{
	printf("***START : write file\n");

	FILE *fp;

	fp = fopen(file_path, "w");
	fprintf(fp, data);
	printf("Data written to file: %s\n", data);
			
	fclose(fp);
	printf("***OVER***\n");
}

/* Read from the file*/
void readFile(const char *file_path, const char *expected)
{
	printf("***START : read file\n");
	
	FILE *fp;
	char data[1024];

	fp = fopen(file_path, "r");

	if (fp == NULL) {
		printf("Error while opening the file.\n");
	} else {
		fscanf(fp, "%s", data);

		if (strcmp(data, expected) == 0)
			printf("Correctly read: %s\n", data);
		else
			printf("Incorrect read: %s\n", data);

		fclose(fp);
	}
	printf("***OVER***\n");
}

/*Copy a file*/
void copyFile(const char *from, const char *to, const char *expected)
{
	printf("***START : copy file\n");
	FILE *f1, *f2;
	char data[1024];
	char ch;

	f1 = fopen(from, "r");
	f2 = fopen(to, "w");

	while((ch = fgetc(f1)) != EOF)
		fputc(ch, f2);

	fclose(f2);
	printf("Copy complete: Testing: ");
	f2 = fopen(to, "r");
	fgets(data, 1084, f2);

	if (strcmp(data, expected) == 0)
		printf("Correct copy: %s\n", data);
	else
		printf("Incorrect copy: %s\n", data);
 
	fclose(f1);
	fclose(f2);
	printf("***OVER***\n");
}

/* get attributes of the file*/
void statTest(const char *file_path)
{
	printf("***START : file stat\n");
	struct stat fileStat;
	
	if(stat(file_path, &fileStat) == -1) { 
		perror("stat");
	}
 
	printf("Information for %s\n", file_path);

	printf("File type:\t");

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
	
	printf("I-node number:              \t%ld\n", (long) fileStat.st_ino);
	
	printf("Mode:                       \t%lo (octal)\n", (unsigned long) fileStat.st_mode);
	
	printf("Number of Hard Links:       \t%ld\n", (long)fileStat.st_nlink);
	
	printf("Ownership:   UID=%ld   GID=%ld\n", (long)fileStat.st_uid, (long)fileStat.st_gid);
	
	printf("I/O Block Size:             \t%ld\n", (long)fileStat.st_blksize);
	
	printf("Number of blocks allocated: \t%ld\n",(long)fileStat.st_blocks);
	
	printf("File Size:                  \t%ld bytes\n",(long)fileStat.st_size);
	
	printf("Last Status Modification:   \t%d\n", ctime(&fileStat.st_ctime));
	
	printf("Last Data Modification:     \t%d\n",ctime(&fileStat.st_mtime));
	
	printf("Last Access:                \t%d\n",ctime(&fileStat.st_atime));
	 
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
	printf("***OVER***\n");
}

/* Read a directory*/
void readDir(const char *dir)
{
	printf("***START : read directory\n");
	DIR *dirp;
	struct dirent *dptr;
	
	if ((dirp=opendir(dir)) == NULL) {
		printf("Error!\n");
		printf("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	} else {
		printf("Directory entries for %s:\n", dir);
		while(dptr = readdir(dirp)) {
			printf("\t%s\n", dptr->d_name);
		}
	}
	printf("***OVER***\n");
}

/*remove a file*/
void removeFile(const char *file_path)
{
	printf("***START : remove file\n");
	int r = remove(file_path);
	if (r == 0 ) {
		printf("file deleted successfully.\n");
	} else {
		printf("Removing file: Failed\n");
		printf("Error no is : %d\n", errno);
		printf("Error description is : %s\n",strerror(errno));
	}
	printf("***OVER***\n");
}

	
/* Rename a file or directory */
/*
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
*/

/* Remove directory*/
/*
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
*/

/* Change permissions of file or directory*/
/*
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
*/

/*Creates 1000 files*/
void multiple(int size)
{
	FILE **fp;
    char filename[20];
    int i;
        
    fp = malloc( sizeof(FILE *) * size);
    
    if( fp != NULL )
    {
        for(i=0; i<size; i++)
        {
             sprintf(filename, "%s%d.txt", "file",i+1);
             
            if( ( fp[i] = fopen(filename, "a+") ) == NULL )
            {
                printf("Error: File \"%s\" cannot be opened\n", filename);
                continue;
            }
	    printf("writing to file %s\n", filename);
			fprintf( fp[i], "%s", "Hello ThereHello There");
            fclose(fp[i]);
        }
	}
		
		else
		{
			printf("Error: No enough memory\n");
			getchar();
		}
    
		free(fp);
		
		printf("All done\n");
}


int main(int argc, char **argv)
{
	if(argc < 2) {
		exit(1);
	}

	char test_dir[] = "fusegitTest";
	char test_file1[] = "fuseFileTest.txt";
	char test_file2[] = "targetFile.txt";
	char test_data[] = "test";
	
	changeDir(argv[1]);
	getPath();
	createDir(test_dir);
	createFile(test_file1);
	writeFile(test_file1, test_data);
	readFile(test_file1, test_data);
	copyFile(test_file1, test_file2, test_data);
	statTest(test_file1);
	readDir(test_dir);
	removeFile(test_file2);
	multiple(5);
	return 0;
}

