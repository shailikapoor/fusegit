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

#include <fuse_opt.h>	// this is to read the input arguments
#include "fg.h"
#include "fg_repo.h"
#include "fg_util.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif






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
	DEBUG("FG_GETATTR");
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
	
	DEBUG("checking if path exists: %s", path);
	if (!repo_path_exists(path))
		return -ENOENT;
	DEBUG("getting attribute of %s", path);
	if ((r = repo_is_dir(path))) {
		//DEBUG("is directory %s", path);
		if ((r = repo_dir_stat(path, stbuf)) < 0)
			return -ENOENT;
		//print_file_stats(path, stbuf);
		return 0;
	}
	if ((r = repo_stat(strcmp("/", path) ? path : "/", stbuf)) < 0)
		return -ENOENT;
	print_file_stats(path, stbuf);
	
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
	DEBUG("FG_READLINK");
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
	DEBUG("FG_MKNOD");
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
	DEBUG("FG_MKDIR");
	int r;
	//DEBUG("directory mode : %o", mode|S_IFDIR);
	
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
	DEBUG("FG_UNLINK");
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
	DEBUG("FG_RMDIR");
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
	DEBUG("FG_SYMLINK");
        return -ENOSYS;
}

/**
 * Rename a file (or directory)
 * 
 * XXX 
 */
static int fg_rename(const char *from, const char *to)
{
	DEBUG("FG_RENAME");
	int r;

	if (repo_is_dir(from)) {
		if ((r = repo_rename_dir(from, to)) < 0)
			return -1;
	} else {
		if ((r = repo_rename_file(from, to)) < 0)
			return -1;
	}
        return 0;
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
	DEBUG("FG_LINK");
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
	DEBUG("FG_CHMOD");
	int r;

	if ((r = repo_chmod(path, mode)) < 0)
		return -ENOLINK;
        return 0;
}

/**
 * Change the owner and group of a file
 * 
 * XXX 
 */
static int fg_chown(const char *path, uid_t uid, gid_t gid)
{
	DEBUG("FG_CHOWN");
	int r;

	if ((r = repo_chown(path, uid, gid)) < 0)
		return -ENOLINK;
        return 0;
}

/**
 * Change the size of a file
 * 
 * XXX 
 */
static int fg_truncate(const char *path, off_t size)
{
	DEBUG("FG_TRUNCATE");
	int r;

	if ((r = repo_truncate(path, size)) < 0)
		return -ENOENT;
        return 0;
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
	DEBUG("FG_OPEN");
	// FIXIT doing nothing for this right now, since we are not using any
	// file handles. Once we start creating file handles, we will need to
	// implement this function.
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
 * If the code comes here, it means that the file exists, since first the code
 * does a getattr on the path.
 */
static int fg_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	DEBUG("FG_READ");
	return repo_read(path, buf, size, offset); 	
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
	DEBUG("FG_WRITE");
        return repo_write(path, buf, size, offset);
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
	DEBUG("FG_STATFS");
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
	DEBUG("FG_RELEASE");
	// FIXIT doing nothing for this right now, since we are not using any
	// file handles. Once we start creating file handles, we will need to
	// implement this function.
        return 0;
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
	DEBUG("FG_FSYNC : metadata?%d", isdatasync?0:1);
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
	DEBUG("FG_SETXATTR");
        return _ENOSYS;
}

/**
 * List extended attributes
 * 
 * XXX 
 */
static int fg_listxattr(const char *path, char *list, size_t size)
{
	DEBUG("FG_LISTXATTR");
        return -ENOSYS;
}

/**
 * Remove extended attributes
 * 
 * XXX 
 */
static int fg_removexattr(const char *path, const char *name)
{
	DEBUG("FG_REMOVEXATTR");
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
	DEBUG("FG_READDIR");
	char name[PATH_MAX_LENGTH] = ""; // name of the path component
	int hier = 0; // stores the index in the hierarchy, starting from 0
	struct repo_file_node *children;
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
	//DEBUG("CHILDREN OBTAINED FROM GIT REPO : %d",
	//	children_count);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	// the filler function takes buf, name of the entry, struct stat of the
	// entry and offset.
	for (i=0; i<children_count; i++) {
		//DEBUG("adding ");
		//DEBUG("%s", children[i].name);
		filler(buf, children[i].name, NULL, 0);
		//DEBUG("added");
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
	DEBUG("FG_ACCESS");
        return -ENOSYS;
}

/**
 * Create and open a file
 * 
 * If the file does not exist, first create it with the specified mode,
 * and then open it.
 * 
 * If this method is not implemented or under Linux kernel versions
 * earlier than 2.6.15, the mknod() and open() methods will be called
 * instead.
 * 
 * Introduced in version 2.5
 */
static int fg_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	DEBUG("FG_CREATE");
	// only when a file doesn't exist will this function be called.
	int r;

	// ask repo to create a empty file and return
	if ((r = repo_create_file(path, mode)) < 0)
		return -1;	// SOME ERROR

	return 0;
}

/**
 * Change the access and modification times of a file with nanosecond reslution
 * 
 * XXX 
 */
static int fg_utimens(const char *path, const struct timespec ts[2])
{
	DEBUG("FG_UTIMENS");
	int r;

	// ask the repo to change the a and m times of the file
	if ((r = repo_update_time_ns(path, ts)) < 0)
		return -1;
        return 0;
}

static struct fuse_operations fg_oper = {
        .getattr        = fg_getattr,	// DONE
        .readlink	= fg_readlink,
        .mknod	        = fg_mknod,
        .mkdir	        = fg_mkdir,	// DONE
        .unlink	        = fg_unlink,	// DONE
        .rmdir	        = fg_rmdir,	// DONE
        .symlink	= fg_symlink,
        .rename	        = fg_rename,
        .link	        = fg_link,	// DONE
        .chmod	        = fg_chmod,
        .chown	        = fg_chown,
        .truncate	= fg_truncate,
        .open	        = fg_open,	// DONE - FIXIT
        .read	        = fg_read,	// DONE
        .write	        = fg_write,
        .statfs	        = fg_statfs,
        //.flush          = fg_flush,	// TODO
        .release	= fg_release,	// DONE - FIXIT
        .fsync	        = fg_fsync,
#ifdef HAVE_SETXATTR
        .setxattr	= fg_setxattr,
        .getxattr	= fg_getxattr,	// TODO
        .listxattr	= fg_listxattr,
        .removexattr	= fg_removexattr,
#endif
        //.opendir        = fg_opendir,	// TODO
        .readdir	= fg_readdir,	// DONE
        //.releasedir	= fg_releasedir,	// TODO
        //.fsyncdir	= fg_fsyncdir,	// TODO
        //.init           = fg_init,	// TODO
        //.destroy        = fg_destroy,	// TODO
        .access	        = fg_access,
        .create         = fg_create,	// DONE
        //.ftruncate      = fg_ftruncate,	// TODO
        //.fgetattr       = fg_fgetattr,	// TODO
        //.lock           = fg_lock,	// TODO
        .utimens	= fg_utimens,	// DONE
        //.bmap           = fg_bmap,	// TODO
        //.ioctl          = fg_ioctl,	// TODO
        //.poll           = fg_poll,	// TODO
};

	int
fg_fuse_main(int argc, char *argv[])
{
	int i;
	for (i=0; i<argc; i++) {
		DEBUG("arg %d: %s", i, argv[i]);
	}
        return fuse_main(argc, argv, &fg_oper, NULL);
}

