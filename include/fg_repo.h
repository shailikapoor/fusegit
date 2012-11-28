/**
 * fg_vcs.h
 */

// ERROR CODES
#define EFG_UNKNOWN	1
#define EFG_NOTEMPTY	2
#define EFG_NOLINK	3

// DATA STRUCTURES


struct fg_file_node {
	const char *name;
	const char *path;
	const struct stat *stbuf;
};


// FUNCTIONS

int repo_setup(const char *repo);

int repo_path_exists(const char *path);

int repo_is_dir(const char *path);

int repo_is_file(const char *path);

int repo_get_children(struct fg_file_node **children, int *count, const char *path);

int repo_stat(const char *path, struct stat *stbuf);

int repo_dir_stat(const char *path, struct stat *stbuf);

int repo_mkdir(const char *path, unsigned int attr);

int repo_rmdir(const char *path);

int repo_link(const char *from, const char *to);

int repo_unlink(const char *path);

int repo_read(const char *path, char *buf, size_t size, off_t offset);

int repo_create_file(const char *path, mode_t mode);

int repo_update_time_ns(const char *path, const struct timespec ts[2]);
