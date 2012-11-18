/**
 * fg_util.h
 */



int is_substr(const char *str, const char *sub);

int get_last_component(const char *path, char *name);

int get_next_component(const char *path, int hier, char *name);

