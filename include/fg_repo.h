/**
 * fg_vcs.h
 */

// ERROR CODES
#define EFG_UNKNOWN	1
#define EFG_NOTEMPTY	2
#define EFG_NOLINK	3

// DATA STRUCTURES


struct repo_file_node {
	const char *name;
	const char *path;
	const struct stat *stbuf;
};

struct repo_stat_data {
	char **links;
	char **expired_links;
	unsigned int count;
	unsigned int ecount;
	unsigned long atime;
	unsigned long mtime;
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

int repo_read(const char *path, char *buf, size_t size, off_t offset);

int repo_create_file(const char *path, mode_t mode);

int repo_update_time_ns(const char *path, const struct timespec ts[2]);

int repo_truncate(const char *path, off_t size);

void free_repo_stat_data(struct repo_stat_data *data);
