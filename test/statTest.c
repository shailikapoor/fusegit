#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv){

	if(argc < 2) 
	{
		exit(1);
	}
	
	struct stat fileStat;
	
	if(stat(argv[1],&fileStat) == -1)
	{ 
		perror("stat");
        exit(EXIT_FAILURE);
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
 
    return 0;
	
}