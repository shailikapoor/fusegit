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

extern void test(void);

static int fg_getattr(const char *path, struct stat *stbuf)
{
        int res = 0;
	test();
        memset(stbuf, 0, sizeof(struct stat));

        if (strcmp(path, "/") == 0) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
        } else {
                res = -ENOENT;
        }
        return res;
}

static int fg_access(const char *path, int mask)
{
        return -ENOSYS;
}

static int fg_readlink(const char *path, char *buf, size_t size)
{
        return -ENOSYS;
}

static int fg_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                         off_t offset, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int fg_mknod(const char *path, mode_t mode, dev_t rdev)
{
        return -ENOSYS;
}

static int fg_mkdir(const char *path, mode_t mode)
{
        return -ENOSYS;
}

static int fg_unlink(const char *path)
{
        return -ENOSYS;
}

static int fg_rmdir(const char *path)
{
        return -ENOSYS;
}

static int fg_symlink(const char *from, const char *to)
{
        return -ENOSYS;
}

static int fg_rename(const char *from, const char *to)
{
        return -ENOSYS;
}

static int fg_link(const char *from, const char *to)
{
        return -ENOSYS;
}

static int fg_chmod(const char *path, mode_t mode)
{
        return -ENOSYS;
}

static int fg_chown(const char *path, uid_t uid, gid_t gid)
{
        return -ENOSYS;
}

static int fg_truncate(const char *path, off_t size)
{
        return -ENOSYS;
}

#ifdef HAVE_UTIMENSAT
static int fg_utimens(const char *path, const struct timespec ts[2])
{
        return -ENOSYS;
}
#endif

static int fg_open(const char *path, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int fg_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int fg_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int fg_statfs(const char *path, struct statvfs *stbuf)
{
        return -ENOSYS;
}

static int fg_release(const char *path, struct fuse_file_info *fi)
{
        return -ENOSYS;
}

static int fg_fsync(const char *path, int isdatasync,
                       struct fuse_file_info *fi)
{
        return -ENOSYS;
}

#ifdef HAVE_SETXATTR
static int fg_setxattr(const char *path, const char *name,
                          const char *value, size_t size, int flags)
{
        return _ENOSYS;
}

static int fg_listxattr(const char *path, char *list, size_t size)
{
        return -ENOSYS;
}

static int fg_removexattr(const char *path, const char *name)
{
        return -ENOSYS;
}
#endif  /* HAVE_SETXATTR */

static struct fuse_operations fg_oper = {
        .getattr        = fg_getattr,
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
