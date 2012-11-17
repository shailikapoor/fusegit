/**
 * fg_git.c
 */
#include <stdio.h>
#include <git2.h>
#include "fg_vcs.h"

static git_repository *repo;

	void
test(void)
{
	// does nothing
}

/**
 * Setup the Git repository at the address repo_address
 */
	int
repo_setup(const char *repo_address)
{
	int r;
	if ((r = git_repository_init(&repo, repo_address, 1)))
		return -1;
	
	return 0;
}
