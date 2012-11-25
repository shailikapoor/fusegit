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
static git_commit *last_commit = NULL;

	static int
l_get_last_commit(git_commit **commit_p)
{
	int r;
	git_oid oid;
	git_reference *ref;
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
	if ((r = git_commit_lookup(commit_p, repo, &oid)) < 0)
		return -1;
	
	return 0;
}

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

	/*
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
	*/
	// get the last commit, i.e. pointing to the reference
	if ((r = l_get_last_commit(&commit)) < 0)
		return -1;
	oid = *git_commit_id(commit);
	
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

	static int
l_get_parent_tree(git_tree **tree, const char *path)
{
	int r;
	char parent[PATH_MAX_LENGTH];

	if ((r = get_parent_path(path, parent)) < 0)
		return -1;

	fprintf(stdout, "Path : %s, Parent path : %s\n", path, parent);
	if ((r = l_get_path_tree(tree, parent)) < 0)
		return r;
	fprintf(stdout, "Parent path obtained\n");
	return 0;
}

	static int
l_git_commit_now(git_tree *tree, const char *message)
{
	int r;
	git_oid oid;
	git_signature *author;
	git_time_t time;
	git_commit *parents[1];

	if (last_commit == NULL) {
		if ((r = l_get_last_commit(&parents[0])) < 0)
			return -1;
	} else {
		parents[0] = last_commit;
	}

	if ((r = git_signature_now(&author, "Varun Agrawal",
		"varun729@gmail.com")) < 0)
		return -1;
	if ((r = git_commit_create(&oid,	// object id
				repo,	// repository
				"HEAD",	// update reference, this will update 
					// the HEAD to this commit
				author,	// author
				author,	// committer
				NULL,	// message encoding, by default UTF-8 is
					// used
				message,	// message for the commit
				tree,	// the git_tree object which will be
					// used as the tree for this commit. 
					// don't know if NULL is valid
				1,	// number of parents. Don't know what
					// value should be used here
				parents)	// array of pointers to the parents
					// (git_commit *parents[])
				) < 0)
		return -1;
	// update the last_commit static variable for this file
	if ((r = git_commit_lookup(&last_commit, repo, &oid)) < 0)
		return -1;
	fprintf(stdout, "l_git_commit_now : Commit successful\n");
	return 0;
}

// ****************************************************************************
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
		return -1;
	
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

/**
 *
 */
 	int
repo_stat(const char *path, struct stat *stbuf)
{
	// Get the git index, using 'git_index_get_bypath' function call and
	// then use the result to obtain the values
	// Following values can be obtained from the index_entry:
	// 	git_index_time ctime
	//	git_index_time mtime
	//	unsigned int dev
	//	unsigned int ino
	//	unsigned int mode
	//	unsigned int uid
	//	unsigned int gid
	//	git_off_t file_size
	//	git_oid oid
	//	unsigned short flags
	//	unsigned short flags_extended
	//	char *path
	// We can set whatever values are required to be set:
	//	struct stat {
	//		dev_t     st_dev;     /* ID of device containing file */
	//		ino_t     st_ino;     /* inode number */
	//		mode_t    st_mode;    /* protection */
	//		nlink_t   st_nlink;   /* number of hard links */
	//		uid_t     st_uid;     /* user ID of owner */
	//		gid_t     st_gid;     /* group ID of owner */
	//		dev_t     st_rdev;    /* device ID (if special file) */
	//		off_t     st_size;    /* total size, in bytes */
	//		blksize_t st_blksize; /* blocksize for file system I/O */
	//		blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
	//		time_t    st_atime;   /* time of last access */
	//		time_t    st_mtime;   /* time of last modification */
	//		time_t    st_ctime;   /* time of last status change */
	//	};
	int r;
	git_index *index;	// TODO manage memory
	git_index_entry *index_entry;
	git_tree *tree;
	int n;

	fprintf(stdout, "GET STAT : %s : STAT : %x\n", path, stbuf);
	//fprintf(stdout, "TRYING TO GET THE TREE\n");
	if ((r = l_get_path_tree(&tree, "/")) < 0)
		return -1;
	if ((r = git_repository_index(&index, repo)) < 0)
		return -1;
	//fprintf(stdout, "OBTAINED THE TREE\n");
	if ((r = git_index_read_tree(index, tree)) < 0)
		return -1;
	//fprintf(stdout, "READING THE TREE IN THE INDEX\n");
	
	path += 1;	// remove the leading '/'
	n = git_index_find(index, path);
	//fprintf(stdout, "INDEX of %s : %d : TOTAL IN INDEX=%d\n", path, n,
	//git_index_entrycount(index));
	if (n < 0)
		return -1;
	index_entry = git_index_get(index, n);

	//fprintf(stdout, "HURRAY!!!!!!!!! %d %x %s %x\n", n, index_entry, path,
	//index);
	stbuf->st_dev = index_entry->dev;     /* ID of device containing file */
	stbuf->st_ino = index_entry->ino;     /* inode number */
	stbuf->st_mode = index_entry->mode;    /* protection */
	stbuf->st_nlink = 1;   /* number of hard links */
	stbuf->st_uid = index_entry->uid;     /* user ID of owner */
	stbuf->st_gid = index_entry->gid;     /* group ID of owner */
	//stbuf->st_rdev = index_entry->rdev;    /* device ID (if special file) */
	stbuf->st_size = index_entry->file_size;    /* total size, in bytes */
	//stbuf->st_blksize = index_entry->blksize; /* blocksize for file system I/O */
	//stbuf->st_blocks = index_entry->blocks;  /* number of 512B blocks allocated */
	//stbuf->st_atime = index_entry->atime;   /* time of last access */
	stbuf->st_mtime = index_entry->mtime.seconds;   /* time of last modification */
	stbuf->st_ctime = index_entry->ctime.seconds;   /* time of last status change */
	
	return 0;
}

/**
 * is the path a directory
 */
	int
repo_isdir(const char *path)
{
	int r;
	git_tree *tree;
	
	if ((r = l_get_path_tree(&tree, path)) == 0)
		return 1;	// is a directory
	return 0;	// is not a directory
}

/**
 * get the stat for a directory
 */
	int
repo_dir_stat(const char *path, struct stat *stbuf)
{
	stbuf->st_dev = 0;     /* ID of device containing file */
	stbuf->st_ino = 0;     /* inode number */
	stbuf->st_mode = S_IFDIR | 0755;    /* protection */
	stbuf->st_nlink = 2;   /* number of hard links */
	stbuf->st_uid = 0;     /* user ID of owner */
	stbuf->st_gid = 0;     /* group ID of owner */
	//stbuf->st_rdev = index_entry->rdev;    /* device ID (if special file) */
	stbuf->st_size = 0;    /* total size, in bytes */
	//stbuf->st_blksize = index_entry->blksize; /* blocksize for file system I/O */
	//stbuf->st_blocks = index_entry->blocks;  /* number of 512B blocks allocated */
	//stbuf->st_atime = index_entry->atime;   /* time of last access */
	stbuf->st_mtime = 0;   /* time of last modification */
	stbuf->st_ctime = 0;   /* time of last status change */
	
	return 0;
}

/**
 * make a directory in the given path
 */
	int
repo_mkdir(const char *path, unsigned int attr)
{
	fprintf(stdout, "In repo_mkdir\n");
	int r;
	git_treebuilder *builder;
	git_treebuilder *empty_treebuilder;
	git_tree_entry *entry;
	git_tree *tree;
	git_oid tree_id;
	git_oid oid;
	char last[PATH_MAX_LENGTH];
	char tmppath[PATH_MAX_LENGTH];
	strcpy(tmppath, path);
	
	while (1) {
		fprintf(stdout, "Getting parent tree\n");
		if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
			return -1;	// tmppath is incorrect
		fprintf(stdout, "Creating tree builder\n");
		if ((r = git_treebuilder_create(&builder, tree)) < 0)
			return -1;	// can't get the treebuilder
		if (strcmp(path, tmppath) == 0) {
			// create a empty tree
			fprintf(stdout, "Creating empty tree builder\n");
			if ((r = git_treebuilder_create(&empty_treebuilder, NULL)) < 0)
				return -1;	// can't create empty tree builder
			fprintf(stdout, "Writing empty tree builder to repo\n");
			if ((r = git_treebuilder_write(&tree_id, repo, empty_treebuilder)) < 0)
				return -1;	// can't insert empty tree into repo
		} else {
			tree_id = oid;
		}
		if ((r = get_last_component(tmppath, last)) < 0)
			return -1;
		fprintf(stdout, "Inserting into tree builder\n");
		if ((r = git_treebuilder_insert(&entry, builder, last, &tree_id, attr))
			< 0)
			return -1;	// can't link the empty tree to repo
		fprintf(stdout, "Writing to the original tree\n");
		if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
			return -1;
		if ((r = get_parent_path(NULL, tmppath)) < 0)
			return -1;	// if tmppath was root, you shouldn't have
					// reached it
		// tmppath is already set to the parent
		if (strlen(tmppath) == 1)
			break;	// the parent of tmppath is /, so break now.
	}

	// TODO get the parrent tree
	// write the code here
	fprintf(stdout, "Getting the tree from the oid\n");
	if ((r = git_tree_lookup(&tree, repo, &oid)) < 0)
		return -1;

	// do the commit
	char header[] = "fusegit\nmkdir\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s", header, path);
	fprintf(stdout, "Making the commit : %s\n", message);
	if ((r = l_git_commit_now(tree, message)) < 0)
		return -1;
	fprintf(stdout, "Commit successful\n");

	// free the tree builder
	git_treebuilder_free(builder);
	git_treebuilder_free(empty_treebuilder);
	return 0;
}
