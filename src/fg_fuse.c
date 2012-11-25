/*
 * Implements the FUSE API function calls
 * 
 * TODO The backup functionality can be called via `fg_fuse` giving it a 
 * parameter like 'restore' or 'backup', to actually restore, or do backup of 
 * the filesystem.
 * TODO it is unknown if I can actually know if the call is made from the current 
 * file system or not. This is annoying. :(
 * 
 * WHENEVER A FILE IS ACCESSED IN THIS FILESYSTEM, IT IS ALWAYS WITH RESPECT TO
 * THE ROOT OF THIS FILE SYSTEM
 */

//#define FUSE_USE_VERSION 29

//#include <fuse.h>
#include <fuse_opt.h>	// this is to read the input arguments
#include <stdio.h>
#include <stdlib.h>	// for exit()
#include <string.h>
#include <errno.h>
#include <unistd.h>	// this is for getcwd
#include "fg.h"
#include "fg_vcs.h"
#include "fg_util.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif



static char FG_ROOT[PATH_MAX_LENGTH];

static const char *fg_path = "/fgtmp";  // path for the /fgtmp file
static const char *fg_str = "Welcome to Fuse GIT";  // content of the /fgtmp file 



//=============================================================================
// FUSE FUNCTIONS
/**
 * Get file attributes. 
 * Returns the metadata about the file specified by the path in a special stat structure.  
 * 
 * Similar to stat(). The 'st_dev' and 'st_blksize' fields are ignored. The
 * 'st_ino' field is ignored exceptif the 'use_ino' mount option is given.
 * 
 * XXX 
 */
static int fg_getattr(const char *path, struct stat *stbuf)
{
	/*
        int res = 0; // temporary result 
	
	memset(stbuf, 0, sizeof(struct stat)); // reset memory and setthe contents of the stat structure to 0

	// We have to check which file attributes we have to return.
	// st_mode in the stat structure will contain the file attributes, S_IFDIR == 0040000 and it marks 
        // and it marks a file a directory. It is thne binary added to 0755 value. 
        if (strcmp(path, "/") == 0) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
        } else if (strcmp(path, "/hihihi") == 0){
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(fg_str);
	} else 
                res = -ENOENT;
	*/

	// Procedure
	// Ask the repository to do this job for you

	// NOTE :
	// st_mode : this is obtained from the tree_entry attribute,
	// 	t_entry->attr
	// st_oid  : this is obtained from the tree_entry oid, t_entry->oid
	int r;
	
	fprintf(stdout, "getting attribute of %s\n", path);
	if ((r = repo_isdir(path))) {
		fprintf(stdout, "is directory %s\n", path);
		if ((r = repo_dir_stat(path, stbuf)) < 0)
			return -ENOENT;
		//print_file_stats(path, stbuf);
		return 0;
	}
	if ((r = repo_stat(strcmp("/", path) ? path : "/.", stbuf)) < 0)
		return -ENOENT;
	//print_file_stats(path, stbuf);
	
        return 0;
}

/**
 * Read the target of the symbolic link
 * 
 * The buffer should be filled with a null terminated string. The buffer size 
 * argument includes the space for the terminating null character. If the 
 * linkname is too long to fit in the buffer, it should be truncated. The 
 * return value should be 0 for success.
 * 
 * XXX 
 */
static int fg_readlink(const char *path, char *buf, size_t size)
{
        return -ENOSYS;
}

/**
 * Create a file node
 * 
 * This is called for creation of all non-directory, non-symlink nodes. If the 
 * filesystem defines a fg_create() method, then for regular files that will be 
 * called instead.
 * 
 * XXX 
 */
static int fg_mknod(const char *path, mode_t mode, dev_t rdev)
{
        return -ENOSYS;
}

/**
 * Create a directory
 * 
 * Note that the mode argument may not have the type specification bits set, 
 * i.e. S_ISDIR(mode) can be false. To obtain the correct directory type bits 
 * use mode|S_IFDIR
 * 
 */
static int fg_mkdir(const char *path, mode_t mode)
{
	int r;
	fprintf(stdout, "directory mode : %o\n", mode|S_IFDIR);
	
	if ((r = repo_mkdir(path, mode|S_IFDIR)) < 0)
		return -ENOENT;
        return 0;
}

/**
 * Remove a file
 * 
 * XXX 
 */
static int fg_unlink(const char *path)
{
	int r;

	if ((r = repo_unlink(path)) < 0)
		return -ENOLINK;
        return 0;
}

/**
 * Remove a directory
 * 
 */
static int fg_rmdir(const char *path)
{
	// FIXIT rmdir can remove empty directories only. But to check this, the
	// tmp_repo should have empty directories. But it is impossible to add
	// an empty directory to a git repository.
	// It is definitely possible to add an empty repository from libgit2,
	// but its possible that the library does some cleaning up when you pack
	// the repository. This needs to be kept in mind.

	// removes an empty directory
	// first check if the directory is empty, if it is empty, delete it
	int r;
	
	if ((r = repo_rmdir(path)) < 0) {
		switch(r) {
		case -EFG_UNKNOWN:
			return -ENOENT;
		case -EFG_NOTEMPTY:
			return -ENOTEMPTY;
		}
		return -ENOENT;
	}
        return 0;
}

/**
 * Create a symbolic link
 * 
 * XXX 
 */
static int fg_symlink(const char *from, const char *to)
{
        return -ENOSYS;
}

/**
 * Rename a file
 * 
 * XXX 
 */
static int fg_rename(const char *from, const char *to)
{
        return -ENOSYS;
}

/**
 * Create a hard link to a file
 * 
 * FIXIT hard links are set, but when you change the content of the file, a new
 * blob will be created, and this will not reflect in the linked file. So take
 * care to actually move the link along with the file.
 */
static int fg_link(const char *from, const char *to)
{
	int r;

	if ((r = repo_link(from, to)) < 0) {
		switch(r) {
		case -EFG_UNKNOWN:
			return -ENOENT;
		case -EFG_NOLINK:
			return -ENOLINK;
		}
		return -ENOLINK;
	}

	return 0;
}

/**
 * Change the permission bits of a file
 * 
 * XXX 
 */
static int fg_chmod(const char *path, mode_t mode)
{
        return -ENOSYS;
}

/**
 * Change the owner and group of a file
 * 
 * XXX 
 */
static int fg_chown(const char *path, uid_t uid, gid_t gid)
{
        return -ENOSYS;
}

/**
 * Change the size of a file
 * 
 * XXX 
 */
static int fg_truncate(const char *path, off_t size)
{
        return -ENOSYS;
}

/**
 * File open operation
 * 
 * No creation (O_CREAT, O_EXCL) and by default also no truncation (O_TRUNC) 
 * flags will be passed to fg_open(). If an application specifies O_TRUNC, fuse 
 * first calls fg_truncate() and then fg_open(). Only if 'atomic_o_trunc' has 
 * been specified, O_TRUNC is passed on to open.
 * 
 * Kernel version 2.6.24 or later
 * 
 * XXX 
 */
static int fg_open(const char *path, struct fuse_file_info *fi)
{
	// TODO all the code in this function is temporary and needs to be look
	// over

	// if the user if asking for anything besides /fgtmp, return  file does not exist
        if(strcmp(path, fg_path) != 0)
		return -ENOENT;
	
	// if the user wants to open the file for anything else than reading only, return  doesn't have sufficient permissions
	if((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	// else open the file
	return 0;
}

/**
 * Read data from an open file
 * 
 * Read should return exactly the number of bytes requested except on EOF 
 * or error, otherwise the rest of the data will be substituted with zeroes. 
 * An exception to this is when the 'direct_io' mount option is specified, in 
 * which case the return value of the read system call will reflect the return 
 * value of this operation.
 * 
 * XXX 
 */
static int fg_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{       
	// TODO all the code in this function is temporary and needs to be look
	// over

	size_t len;
	(void) fi;

	if(strcmp(path, fg_path) != 0){
		return -ENOENT;
	}

	if(strcmp(path, fg_path) == 0){
		len = strlen(fg_str);
		if(offset < len){
			if(offset + size > len)
				size = len - offset;
			memcpy(buf, fg_str + offset, size);
		}else
			size=0;
		
	}

        return size;
}

/**
 * Write data to an open file
 * 
 * Write should return exactly the number of bytes requested except on error. An 
 * exception to this is when the 'direct_io' mount option is specified.
 * 
 * XXX 
 */
static int fg_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

/**
 * Get file system statistics
 * 
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored.
 * 
 * XXX 
 */
static int fg_statfs(const char *path, struct statvfs *stbuf)
{
        return -ENOSYS;
}

/**
 * Release an open file
 * 
 * Release is called when there are no more references to an open file: all 
 * file descriptors are closed and all memory mappings are unmapped.
 * 
 * For every fg_open() call there will be exactly one fg_release() call with 
 * the same flags and file descriptor. It is possible to have a file opened 
 * more than once, in which case only the last release will mean, that no 
 * more reads/writes will happen on the file. The return value of release is 
 * ignored.
 * 
 * XXX 
 */
static int fg_release(const char *path, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

/**
 * Synchronize the file contents
 * 
 * If the datasync parameter is non-zero, then only the user data should be 
 * flushed, not the meta data.
 * 
 * XXX 
 */
static int fg_fsync(const char *path, int isdatasync,
                       struct fuse_file_info *fi)
{
        return -ENOSYS;
}

#ifdef HAVE_SETXATTR
/**
 * Set extended attributes
 * 
 * XXX 
 */
static int fg_setxattr(const char *path, const char *name,
                          const char *value, size_t size, int flags)
{
        return _ENOSYS;
}

/**
 * List extended attributes
 * 
 * XXX 
 */
static int fg_listxattr(const char *path, char *list, size_t size)
{
        return -ENOSYS;
}

/**
 * Remove extended attributes
 * 
 * XXX 
 */
static int fg_removexattr(const char *path, const char *name)
{
        return -ENOSYS;
}
#endif  /* HAVE_SETXATTR */

/**
 * Read directory
 * 
 * The filesystem may choose between two modes of operation.
 * 
 * 1) The readdir implementation ignores the offset parameter, and passes zero 
 * to the filler function's offset. The filler function will not return '1' 
 * (unless an error happens), so the whole directory is read in a single 
 * readdir operation.
 * 
 * 2) The readdir implementation keeps track of the offsets of the directory 
 * entries. It uses the offset parameter and always passes non-zero offset to 
 * the filler function. When the buffer is full (or an error happens) the 
 * filler function will return '1'.
 * 
 * XXX 
 */
static int fg_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                         off_t offset, struct fuse_file_info *fi)
{
	// XXX no need to check if the path is for this filesystem
	char name[PATH_MAX_LENGTH] = ""; // name of the path component
	int hier = 0; // stores the index in the hierarchy, starting from 0
	struct fg_file_node *children;
	int children_count;
	int i, r;
	
	while (get_next_component(path, hier++, name)) {
		// if the path doesn't exist then return -ENOENT
		if (!repo_path_exists(name))
			return -ENOENT;
	}
	// if the path doesn't exist then return -ENOENT
	if (repo_is_file(name))
		return -ENOENT;
	
	// NOTE : this function implements the first mode of operation, where
	// offset is ignored.

	// find the contents of the directory and then call the filler function
	// for each of the entries one by one.
	if ((r = repo_get_children(&children, &children_count, path)) < 0)
		return -ENOENT;
	fprintf(stdout, "CHILDREN OBTAINED FROM GIT REPO : %d\n",
		children_count);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	// the filler function takes buf, name of the entry, struct stat of the
	// entry and offset.
	for (i=0; i<children_count; i++) {
		fprintf(stdout, "adding ");
		fprintf(stdout, "%s\n", children[i].name);
		filler(buf, children[i].name, NULL, 0);
		fprintf(stdout, "added\n");
	}
	free(children);

        return 0;
}

/**
 * Check file access permissions
 *
 * This will be called for the fg_access() system call. If the 
 * 'default_permissions' mount option is given, this method is not called.
 * 
 * Kernel version 2.4.x above
 * 
 * XXX 
 */
static int fg_access(const char *path, int mask)
{
        return -ENOSYS;
}

#ifdef HAVE_UTIMENSAT
/**
 * Change the access and modification times of a file with nanosecond reslution
 * 
 * XXX 
 */
static int fg_utimens(const char *path, const struct timespec ts[2])
{
        return -ENOSYS;
}
#endif

static struct fuse_operations fg_oper = {
        .getattr        = fg_getattr,
        .readlink	= fg_readlink,
        .mknod	        = fg_mknod,
        .mkdir	        = fg_mkdir,
        .unlink	        = fg_unlink,
        .rmdir	        = fg_rmdir,
        .symlink	= fg_symlink,
        .rename	        = fg_rename,
        .link	        = fg_link,
        .chmod	        = fg_chmod,
        .chown	        = fg_chown,
        .truncate	= fg_truncate,
        .open	        = fg_open,
        .read	        = fg_read,
        .write	        = fg_write,
        .statfs	        = fg_statfs,
        //.flush          = fg_flush, // TODO
        .release	= fg_release,
        .fsync	        = fg_fsync,
#ifdef HAVE_SETXATTR
        .setxattr	= fg_setxattr,
        .getxattr	= fg_getxattr, // TODO
        .listxattr	= fg_listxattr,
        .removexattr	= fg_removexattr,
#endif
        //.opendir        = fg_opendir, // TODO
        .readdir	= fg_readdir,
        //.releasedir	= fg_releasedir, // TODO
        //.fsyncdir	= fg_fsyncdir, // TODO
        //.init           = fg_init, // TODO
        //.destroy        = fg_destroy, // TODO
        .access	        = fg_access,
        //.create         = fg_create, // TODO
        //.ftruncate      = fg_ftruncate, // TODO
        //.fgetattr       = fg_fgetattr, // TODO
        //.lock           = fg_lock, // TODO
#ifdef HAVE_UTIMENSAT
        .utimens	= fg_utimens,
#endif
        //.bmap           = fg_bmap, // TODO
        //.ioctl          = fg_ioctl, // TODO
        //.poll           = fg_poll, // TODO
};

/**
 * remove any trailing / from the mpoint which is returned
 */
	static void
get_mountpoint(const char *arg, char *mpoint)
{
	char cwd[PATH_MAX_LENGTH];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
		printf("current working directory: %s\n", cwd);
	printf("function is called for %s\n", arg);

	const char *y = cwd;
	while ((*mpoint = *y)) {
		mpoint++;
		y++;
	}
	y = arg;
	*mpoint = '/'; mpoint++;
	while ((*mpoint = *y)) {
		mpoint++;
		y++;
	}
}

/**
 * Process the mountpoint which the user has provided and then use it to create
 * a git repository in the same parent folder.
 */
	static int
proc_mountpoint(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	int r;
	static int times_called = 0;
	if (times_called++ > 0)
		return -1;
	
	// Now that we have checked that this is the only output we expect, i.e.
	// this is the mountpoint of our filesystem. Check if a git repository
	// already exists for this or not.
	// If (git repository exists for this mountpoint)
	// 	setup the file system to use the git repository
	// Else
	// 	create a git repository and setup the file system to use the git
	// 	repository.
	char repo[PATH_MAX_LENGTH];
	get_mountpoint(arg, repo);
	strcpy(FG_ROOT, repo);
	printf("mountpoint is %s\n", repo);
	if (strlen(repo) + strlen(".repo") >= PATH_MAX_LENGTH-1)
		exit(-1);
	strcpy(repo+strlen(repo), ".repo");
	printf("repository is %s\n", repo);

	// Now we have obtained the address of the repository, we can set it
	if ((r = repo_setup(repo)) < 0)
		exit(r);

	return 0;
}

	int
main(int argc, char *argv[])
{
        umask(0); // XXX this provides the bits which will be masked for all the files which are to be created.

	// creating the fuse_args to parse it for the mount-point
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_opt matching_opts[] = { FUSE_OPT_KEY("foo", 0) };
	fuse_opt_parse(&args, NULL, matching_opts, (fuse_opt_proc_t)proc_mountpoint);

        return fuse_main(argc, argv, &fg_oper, NULL);
}
