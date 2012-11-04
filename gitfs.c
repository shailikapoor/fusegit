/*
 * Copyright
 *
 * License
 *
 * gcc -Wall `pkg-config fuse --cflags --libs` gitfs.c -o gitfs
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

static int gitfs_getattr(const char *path, struct stat *stbuf)
{
        int res = 0;

        memset(stbuf, 0, sizeof(struct stat));

        if (strcmp(path, "/") == 0) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
        } else {
                res = -ENOENT;
        }
        return res;
}

static int gitfs_access(const char *path, int mask)
{
        return -ENOSYS;
}

static int gitfs_readlink(const char *path, char *buf, size_t size)
{
        return -ENOSYS;
}

static int gitfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                         off_t offset, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int gitfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
        return -ENOSYS;
}

static int gitfs_mkdir(const char *path, mode_t mode)
{
        return -ENOSYS;
}

static int gitfs_unlink(const char *path)
{
        return -ENOSYS;
}

static int gitfs_rmdir(const char *path)
{
        return -ENOSYS;
}

static int gitfs_symlink(const char *from, const char *to)
{
        return -ENOSYS;
}

static int gitfs_rename(const char *from, const char *to)
{
        return -ENOSYS;
}

static int gitfs_link(const char *from, const char *to)
{
        return -ENOSYS;
}

static int gitfs_chmod(const char *path, mode_t mode)
{
        return -ENOSYS;
}

static int gitfs_chown(const char *path, uid_t uid, gid_t gid)
{
        return -ENOSYS;
}

static int gitfs_truncate(const char *path, off_t size)
{
        return -ENOSYS;
}

#ifdef HAVE_UTIMENSAT
static int gitfs_utimens(const char *path, const struct timespec ts[2])
{
        return -ENOSYS;
}
#endif

static int gitfs_open(const char *path, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int gitfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int gitfs_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int gitfs_statfs(const char *path, struct statvfs *stbuf)
{
        return -ENOSYS;
}

static int gitfs_release(const char *path, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int gitfs_fsync(const char *path, int isdatasync,
                       struct fuse_file_info *fi)
{
        return -ENOSYS;
}

#ifdef HAVE_SETXATTR
static int gitfs_setxattr(const char *path, const char *name,
                          const char *value, size_t size, int flags)
{
        return _ENOSYS;
}

static int gitfs_listxattr(const char *path, char *list, size_t size)
{
        return -ENOSYS;
}

static int gitfs_removexattr(const char *path, const char *name)
{
        return -ENOSYS;
}
#endif  /* HAVE_SETXATTR */

static struct fuse_operations gitfs_oper = {
        .getattr        = gitfs_getattr,
        .access	        = gitfs_access,
        .readlink	= gitfs_readlink,
        .readdir	= gitfs_readdir,
        .mknod	        = gitfs_mknod,
        .mkdir	        = gitfs_mkdir,
        .symlink	= gitfs_symlink,
        .unlink	        = gitfs_unlink,
        .rmdir	        = gitfs_rmdir,
        .rename	        = gitfs_rename,
        .link	        = gitfs_link,
        .chmod	        = gitfs_chmod,
        .chown	        = gitfs_chown,
        .truncate	= gitfs_truncate,
#ifdef HAVE_UTIMENSAT
        .utimens	= gitfs_utimens,
#endif
        .open	        = gitfs_open,
        .read	        = gitfs_read,
        .write	        = gitfs_write,
        .statfs	        = gitfs_statfs,
        .release	= gitfs_release,
        .fsync	        = gitfs_fsync,
#ifdef HAVE_SETXATTR
        .setxattr	= gitfs_setxattr,
        .getxattr	= gitfs_getxattr,
        .listxattr	= gitfs_listxattr,
        .removexattr	= gitfs_removexattr,
#endif
};

int main(int argc, char *argv[])
{
        umask(0);
        return fuse_main(argc, argv, &gitfs_oper, NULL);
}
