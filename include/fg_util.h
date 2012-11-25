/**
 * fg_util.h
 */



int is_substr(const char *str, const char *sub);

int get_parent_path(const char *path, char *parent);

int get_last_component(const char *path, char *name);

int get_next_component(const char *path, int hier, char *name);

void print_file_stats(const char *path, struct stat *stbuf);
