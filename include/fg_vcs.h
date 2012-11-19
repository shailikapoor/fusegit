/**
 * fg_vcs.h
 */

// DATA STRUCTURES
struct fg_stat {

};

struct fg_file_node {
	const char *name;
	const char *path;
	const struct fg_stat *stat;
};


// FUNCTIONS

int repo_setup(const char *repo);

int repo_path_exists(const char *path);

int repo_is_file(const char *path);

int repo_get_children(struct fg_file_node **children, int *count, const char *path);
