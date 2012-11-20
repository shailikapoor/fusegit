/**
 * fg_git.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <git2.h>
#include "fg.h"
#include "fg_vcs.h"
#include "fg_util.h"

// LOCAL
static git_repository *repo;

	static int
l_get_path_tree(git_tree **tree, const char *path)
{
	git_oid oid;
	git_reference *ref;	// TODO how to manage the pointer
	git_commit *commit;	// TODO how to manage the pointer
	git_tree *l_tree;	// TODO how to manage the pointer
	git_tree_entry *tree_entry;	// TODO how to manage the pointer
	git_object *object;	// TODO how to manage the pointer

	char name[PATH_MAX_LENGTH];
	char last[PATH_MAX_LENGTH];
	int hier;
	int r;

	// obtaining the head
	fprintf(stdout, "OBTAINING THE HEAD\n");
	if ((r = git_repository_head(&ref, repo)) < 0)
		return -1;

	// obtaining the commit id from the reference
	fprintf(stdout, "OBTAINING commit id from the reference\n");
	if ((r = git_reference_name_to_oid(&oid, repo, git_reference_name(ref)))
		< 0)
		return -1;

	// obtaining the commit from the commit id
	fprintf(stdout, "OBTAINING commit from the commit id\n");
	if ((r = git_commit_lookup(&commit, repo, &oid)) < 0)
		return -1;
	
	// obtaining the tree id from the commit
	fprintf(stdout, "OBTAINING tree id from the commit\n");
	oid = *git_commit_tree_oid(commit);

	// obtaining the tree from the tree id
	fprintf(stdout, "OBTAINING tree from tree id\n");
	if ((r = git_tree_lookup(&l_tree, repo, &oid)) < 0)
		return -1;

	// TODO following is the algorithm remaining to be implemented
	hier = 0;
	*name = '\0';
	while ((r = get_next_component(path, hier++, name)) != 0) {
		get_last_component(name, last);
		if (*last == '\0')
			continue;
		// l_tree is the tree for the current path

		fprintf(stdout, "OBTAINING tree entry for %s\n", name);
		tree_entry = git_tree_entry_byname(l_tree, last);
		if (tree_entry == NULL)
			return -1;
		if ((r = git_tree_entry_to_object(&object, repo, tree_entry)) <
			0)
			return -1;
		if (git_object_type(object) != GIT_OBJ_TREE)
			return -1;
		oid = *git_tree_entry_id(tree_entry);
		if ((r = git_tree_lookup(&l_tree, repo, &oid)) < 0)
			return -1;
	}
	*tree = l_tree;
	fprintf(stdout, "SUCCESSFUL\n");

	// while (l_tree is not the last tree) {
	// 	n = git_tree_entrycount(l_tree);
	//	req_comp = next component in path;
	//	for (i=0; i<n; i++) {
	//		entry = get_tree_entry(i);
	//		if (entry is req_comp)
	//			break;
	//	}
	//	l_tree = tree corresponding to the entry
	// }
	// *tree = l_tree;

	return 0;
}


// GLOBAL

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

/**
 * returns the children of the directory represented by path
 * 
 * Note : children should be freed from the function which calls it.
 */
	int
repo_get_children(struct fg_file_node **children, int *count, const char *path)
{
	int i, r, n;
	git_tree *tree;	// TODO check how to manage memory
	git_tree_entry *entry;	// TODO check how to manage memory

	fprintf(stdout, "ENTERING THE l_get_path_tree: \"%s\"\n", path);
	if ((r = l_get_path_tree(&tree, path)) < 0)
		return NULL;
	
	n = git_tree_entrycount(tree);
	struct fg_file_node *nodes = malloc(n * sizeof(struct fg_file_node));
	*children = nodes;	// setting the children array
	*count = n;	// setting the number of children

	for (i=0; i<n; i++) {
		entry = git_tree_entry_byindex(tree, i);
		nodes[i].name = git_tree_entry_name(entry);
		fprintf(stdout, "added.... %s\n", nodes[i].name);
	}
	fprintf(stdout, "FINISHED GETTING CHILDREN\n");
	return 0;
}
