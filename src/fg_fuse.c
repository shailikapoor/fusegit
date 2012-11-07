/*
 * Copyright
 *
 * License
 *
 * gcc -Wall `pkg-config fuse --cflags --libs` fg_fuse.c -o fg_fuse
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "fg_vcs.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

extern void test(void); // TODO remove this. This is just to test that the files in src folder and include folder are linked properly.

/**
 * Get file attributes.
 * 
 * Similar to stat(). The 'st_dev' and 'st_blksize' files are ignored. The
 * 'st_ino' field is ignored exceptif the 'use_ino' mount option is given.
 */
static int fg_getattr(const char *path, struct stat *stbuf)
{
        int res = 0;
	test(); // TODO remove this. This is just to test that the files in src folder and include folder are linked properly.
        memset(stbuf, 0, sizeof(struct stat));

        if (strcmp(path, "/") == 0) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
        } else {
                res = -ENOENT;
        }
        return res;
}

/**
 * Check file access permissions
 *
 * This will be called for the fg_access() system call. If the 
 * 'default_permissions' mount option is given, this method is not called.
 * 
 * Kernel version 2.4.x above
 */
static int fg_access(const char *path, int mask)
{
        return -ENOSYS;
}

/**
 * Read the target of the symbolic link
 * 
 * The buffer should be filled with a null terminated string. The buffer size 
 * argument includes the space for the terminating null character. If the 
 * linkname is too long to fit in the buffer, it should be truncated. The 
 * return value should be 0 for success.
 */
static int fg_readlink(const char *path, char *buf, size_t size)
{
        return -ENOSYS;
}

/**
 * Read directory
 * 
 * The filesystem may choose between two modes of operation.
 * 
 * 1) The readdir implementation ignores the offset parameter, and passes zero 
 * to the filler function's offset. The killer function will not return '1' 
 * (unless an error happens), so the whole directory is read in a single 
 * readdir operation.
 * 
 * 2) The readdir implementation keeps track of the offsets of the directory 
 * entries. It uses the offset parameter and always passes non-zero offset to 
 * the filler function. When the buffer is full (or an error happens) the 
 * filler function will return '1'.
 */
static int fg_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                         off_t offset, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

/**
 * Create a file node
 * 
 * This is called for creation of all non-directory, non-symlink nodes. If the 
 * filesystem defines a fg_create() method, then for regular files that will be 
 * called instead.
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
 */
static int fg_mkdir(const char *path, mode_t mode)
{
        return -ENOSYS;
}

/**
 * Remove a file
 */
static int fg_unlink(const char *path)
{
        return -ENOSYS;
}

/**
 * Remove a directory
 */
static int fg_rmdir(const char *path)
{
        return -ENOSYS;
}

/**
 * Create a symbolic link
 */
static int fg_symlink(const char *from, const char *to)
{
        return -ENOSYS;
}

/**
 * Rename a file
 */
static int fg_rename(const char *from, const char *to)
{
        return -ENOSYS;
}

/**
 * Create a hard link to a file
 */
static int fg_link(const char *from, const char *to)
{
        return -ENOSYS;
}

/**
 * Change the permission bits of a file
 */
static int fg_chmod(const char *path, mode_t mode)
{
        return -ENOSYS;
}

/**
 * Change the owner and group of a file
 */
static int fg_chown(const char *path, uid_t uid, gid_t gid)
{
        return -ENOSYS;
}

/**
 * Change the size of a file
 */
static int fg_truncate(const char *path, off_t size)
{
        return -ENOSYS;
}

#ifdef HAVE_UTIMENSAT
/**
 * Change the access and modification times of a file with nanosecond reslution
 */
static int fg_utimens(const char *path, const struct timespec ts[2])
{
        return -ENOSYS;
}
#endif

/**
 * File open operation
 * 
 * No creation (O_CREAT, O_EXCL) and by default also no truncation (O_TRUNC) 
 * flags will be passed to fg_open(). If an application specifies O_TRUNC, fuse 
 * first calls fg_truncate() and then fg_open(). Only if 'atomic_o_trunc' has 
 * been specified, O_TRUNC is passed on to open.
 * 
 * Kernel version 2.6.24 or later
 */
static int fg_open(const char *path, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

/**
 * Read data from an open file
 * 
 * Read should return exactly the number of bytes requested except on EOF 
 * or error, otherwise the rest of the data will be substituted with zeroes. 
 * An exception to this is when the 'direct_io' mount option is specified, in 
 * which case the return value of the read system call will reflect the return 
 * value of this operation.
 */
static int fg_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
        return -ENOSYS;
}

/**
 * Write data to an open file
 * 
 * Write should return exactly the number of bytes requested except on error. An 
 * exception to this is when the 'direct_io' mount option is specified.
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
 */
static int fg_fsync(const char *path, int isdatasync,
                       struct fuse_file_info *fi)
{
        return -ENOSYS;
}

#ifdef HAVE_SETXATTR
/**
 * Set extended attributes
 */
static int fg_setxattr(const char *path, const char *name,
                          const char *value, size_t size, int flags)
{
        return _ENOSYS;
}

/**
 * List extended attributes
 */
static int fg_listxattr(const char *path, char *list, size_t size)
{
        return -ENOSYS;
}

/**
 * Remove extended attributes
 */
static int fg_removexattr(const char *path, const char *name)
{
        return -ENOSYS;
}
#endif  /* HAVE_SETXATTR */

static struct fuse_operations fg_oper = {
        .getattr        = fg_getattr, // get file attributes
        .access	        = fg_access,
        .readlink	= fg_readlink,
        .readdir	= fg_readdir,
        .mknod	        = fg_mknod,
        .mkdir	        = fg_mkdir,
        .symlink	= fg_symlink,
        .unlink	        = fg_unlink,
        .rmdir	        = fg_rmdir,
        .rename	        = fg_rename,
        .link	        = fg_link,
        .chmod	        = fg_chmod,
        .chown	        = fg_chown,
        .truncate	= fg_truncate,
#ifdef HAVE_UTIMENSAT
        .utimens	= fg_utimens,
#endif
        .open	        = fg_open,
        .read	        = fg_read,
        .write	        = fg_write,
        .statfs	        = fg_statfs,
        .release	= fg_release,
        .fsync	        = fg_fsync,
#ifdef HAVE_SETXATTR
        .setxattr	= fg_setxattr,
        .getxattr	= fg_getxattr,
        .listxattr	= fg_listxattr,
        .removexattr	= fg_removexattr,
#endif
};

int main(int argc, char *argv[])
{
        umask(0);
        return fuse_main(argc, argv, &fg_oper, NULL);
}
