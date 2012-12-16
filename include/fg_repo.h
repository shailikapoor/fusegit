/**
 * fg_vcs.h
 */

// ERROR CODES
#define EFG_UNKNOWN	1
#define EFG_NOTEMPTY	2
#define EFG_NOLINK	3
#define EFG_MAX_HANDLES	4
#define EFG_EMPTY_HANDLES	5

#define MAX_HANDLES	1024
#define FILE_BUFFER_CHUNK	4096
// DATA STRUCTURES


struct repo_file_node {
	const char *name;
	const char *path;
	const struct stat *stbuf;
};

struct repo_stat_data {
	unsigned int count;
	unsigned int ecount;
	unsigned long atime;
	unsigned long mtime;
	uid_t uid;
	gid_t gid;
	char **links;
	char **expired_links;
};

// data is stored in multiples of FILE_BUFFER_CHUNK only
// OBSOLETE
// The file handle should not have the information of the path, since this
// prevent us from using the handle, if the file is moved to a different
// location.
struct repo_file_handle {
	int free;
	char *path;
	char buf[FILE_BUFFER_CHUNK];
	size_t size;
	off_t off;
	int dirty;
	pthread_mutex_t lock;
};

struct repo_file_handle_queue {
	struct repo_file_handle *handles[MAX_HANDLES];
	int full;
	int empty;
	int next;
	pthread_mutex_t lock;
};

// FUNCTIONS

int repo_setup(const char *repo);

int repo_path_exists(const char *path);

int repo_is_dir(const char *path);

int repo_is_file(const char *path);

int repo_get_children(struct repo_file_node **children, int *count, const char *path);

int repo_stat(const char *path, struct stat *stbuf);

int repo_dir_stat(const char *path, struct stat *stbuf);

int repo_mkdir(const char *path, unsigned int attr);

int repo_rmdir(const char *path);

int repo_link(const char *from, const char *to);

int repo_unlink(const char *path);

int repo_update_time_ns(const char *path, const struct timespec ts[2]);

int repo_truncate(const char *path, off_t size);

void free_repo_stat_data(struct repo_stat_data *data);

int repo_rename_file(const char *from, const char *to);

int repo_rename_dir(const char *from, const char *to);

int repo_backup(const char *snapshot);

int repo_restore(const char *snapshot);

int repo_chmod(const char *path, mode_t mode);

int repo_chown(const char *path, uid_t uid, gid_t gid);

int repo_open(const char *path, uint64_t *fh);

int repo_release(const char *path, uint64_t fh);

int repo_flush(const char *path, uint64_t fh);

int repo_fsync(const char *path, uint64_t fh, int isdatasync);

int repo_create_file(const char *path, mode_t mode, uint64_t *fh);

int repo_read(const char *path, char *buf, size_t size, off_t offset, uint64_t
	fh);

int repo_write(const char *path, const char *buf, size_t size, off_t offset,
	uint64_t fh);
