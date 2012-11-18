/**
 * fg_git.c
 */
#include <stdio.h>
#include <git2.h>
#include "fg_vcs.h"

static git_repository *repo;

/**
 * Setup the Git repository at the address repo_address
 */
	int
repo_setup(const char *repo_address)
{
	int r;
	if ((r = git_repository_init(&repo, repo_address, 1)))
		return -1;
	// TODO test if the repository already exists, if it already exists then
	// just use it, and don't open a new repository.
	// TODO use the location specified by the config file to store the
	// repository in the parent file system
	return 0;
}

/**
 * checks if the path exists in the repository
 * returns 1 if the path exists, else 0
 */
	int
repo_path_exists(const char *path)
{
	// TODO
	return 1;
}

/**
 * checks if the path is of a file
 * returns 1 if it is file, else 0
 */
	int
repo_is_file(const char *path)
{
	// TODO
	return 0;
}
