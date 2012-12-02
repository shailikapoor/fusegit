/**
 * fg_git.c
 */
#include <git2.h>
#include "fg.h"
#include "fg_repo.h"
#include "fg_util.h"

// LOCAL
static const unsigned int INVALID_FILE_MODE = 077777777;
static git_repository *repo;
static git_commit *last_commit = NULL;
char note_data[PATH_MAX_LENGTH*MAX_HARD_LINKS];

	static int
l_get_last_commit(git_commit **commit_p)
{
	int r;
	git_oid oid;
	git_reference *ref;
	// obtaining the head
	//DEBUG("OBTAINING THE HEAD");
	if ((r = git_repository_head(&ref, repo)) < 0)
		return -EFG_UNKNOWN;

	// obtaining the commit id from the reference
	//DEBUG("OBTAINING commit id from the reference");
	if ((r = git_reference_name_to_oid(&oid, repo, git_reference_name(ref)))
		< 0)
		return -EFG_UNKNOWN;

	// obtaining the commit from the commit id
	//DEBUG("OBTAINING commit from the commit id");
	if ((r = git_commit_lookup(commit_p, repo, &oid)) < 0)
		return -EFG_UNKNOWN;
	
	return 0;
}

	static int
l_get_path_tree(git_tree **tree, const char *path)
{
	git_oid oid;
	git_commit *commit;	// TODO how to manage the pointer
	git_tree *l_tree;	// TODO how to manage the pointer
	const git_tree_entry *tree_entry;	// TODO how to manage the pointer
	git_object *object;	// TODO how to manage the pointer

	char name[PATH_MAX_LENGTH];
	char last[PATH_MAX_LENGTH];
	int hier;
	int r;

	// get the last commit, i.e. pointing to the reference
	if ((r = l_get_last_commit(&commit)) < 0)
		return -EFG_UNKNOWN;
	oid = *git_commit_id(commit);
	
	// obtaining the tree id from the commit
	//DEBUG("OBTAINING tree id from the commit");
	oid = *git_commit_tree_oid(commit);

	// obtaining the tree from the tree id
	//DEBUG("OBTAINING tree from tree id");
	if ((r = git_tree_lookup(&l_tree, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	// TODO following is the algorithm remaining to be implemented
	hier = 0;
	*name = '\0';
	while ((r = get_next_component(path, hier++, name)) != 0) {
		get_last_component(name, last);
		if (*last == '\0')
			continue;
		// l_tree is the tree for the current path

		//DEBUG("OBTAINING tree entry for %s", name);
		tree_entry = git_tree_entry_byname(l_tree, last);
		if (tree_entry == NULL)
			return -EFG_UNKNOWN;
		if ((r = git_tree_entry_to_object(&object, repo, tree_entry)) <
			0)
			return -EFG_UNKNOWN;
		if (git_object_type(object) != GIT_OBJ_TREE)
			return -EFG_UNKNOWN;
		oid = *git_tree_entry_id(tree_entry);
		if ((r = git_tree_lookup(&l_tree, repo, &oid)) < 0)
			return -EFG_UNKNOWN;
	}
	*tree = l_tree;
	//DEBUG("SUCCESSFUL");
	
	return 0;
}

	static int
l_get_parent_tree(git_tree **tree, const char *path)
{
	int r;
	char parent[PATH_MAX_LENGTH];

	if ((r = get_parent_path(path, parent)) < 0)
		return -EFG_UNKNOWN;

	//DEBUG("Path : %s, Parent path : %s", path, parent);
	if ((r = l_get_path_tree(tree, parent)) < 0)
		return r;
	//DEBUG("Parent path obtained");
	return 0;
}

	static int
l_get_signature_now(git_signature **author_p)
{
	int r;
	if ((r = git_signature_now(author_p, "Varun Agrawal",
		"varun729@gmail.com")) < 0)
		return -EFG_UNKNOWN;
	return 0;
}

	static int
l_git_commit_now(git_tree *tree, const char *message)
{
	int r;
	git_oid oid;
	git_signature *author;
	git_commit *tmp;
	const git_commit *parents[1];
	int num_parents = 1;

	if (last_commit == NULL) {
		if ((r = l_get_last_commit(&tmp)) < 0) {
			// NEW REPOSITORY
			num_parents = 0;
		} else {
			parents[0] = tmp;
		}
	} else {
		parents[0] = last_commit;
	}

	if ((r = l_get_signature_now(&author)) < 0)
		return -EFG_UNKNOWN;
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
				num_parents,	// number of parents. Don't know what
					// value should be used here
				parents)	// array of pointers to the parents
					// (git_commit *parents[])
				) < 0)
		return -EFG_UNKNOWN;
	// update the last_commit static variable for this file
	if ((r = git_commit_lookup(&last_commit, repo, &oid)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("l_git_commit_now : Commit successful");
	return 0;
}

	static int
l_make_commit(const char *path, git_oid oid, const char *message)
{
	int r;
	git_treebuilder *builder;
	git_tree *tree;
	unsigned int attr = S_IFDIR | 0755;	// FIXIT HARD CODED
	char last[PATH_MAX_LENGTH];
	char tmppath[PATH_MAX_LENGTH];
	strcpy(tmppath, path);

	// remove the directory entry from parent, and move up the trees
	while (strlen(tmppath) > 1) {
		//DEBUG("Getting parent tree");
		if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
			return -EFG_UNKNOWN;	// tmppath is incorrect
		//DEBUG("Creating tree builder");
		if ((r = git_treebuilder_create(&builder, tree)) < 0)
			return -EFG_UNKNOWN;	// can't get the treebuilder
		if ((r = get_last_component(tmppath, last)) < 0)
			return -EFG_UNKNOWN;
		//DEBUG("Inserting into tree builder");
		if ((r = git_treebuilder_insert(NULL, builder, last, &oid, attr))
			< 0)
			return -EFG_UNKNOWN;	// can't link the empty tree to repo
		//DEBUG("Writing to the original tree");
		if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
			return -EFG_UNKNOWN;
		// free the builder
		git_treebuilder_free(builder);
		if ((r = get_parent_path(NULL, tmppath)) < 0)
			return -EFG_UNKNOWN;	// if tmppath was root, you shouldn't have reached it
	}

	// get the parrent tree
	//DEBUG("Getting the tree from the oid");
	//printf("look up the tree\n");
	if ((r = git_tree_lookup(&tree, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	//printf("commit the tree to repo\n");
	if ((r = l_git_commit_now(tree, message)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Commit successful");
	//printf("commit successful\n");

	return 0;
}

	static size_t
l_get_file_size(const char *path)
{
	int r;
	git_tree *tree;
	const git_tree_entry * entry;
	git_oid oid;
	git_blob *blob;
	char last[PATH_MAX_LENGTH];
	size_t size;
	
	if ((r = l_get_parent_tree(&tree, path)) < 0)
		return -1;
	if ((r = get_last_component(path, last)) < 0)
		return -1;
	entry = git_tree_entry_byname(tree, last);
	if (entry == NULL)
		return -1;
	oid = *git_tree_entry_id(entry);
	if (git_tree_entry_type(entry) != GIT_OBJ_BLOB)
		return -1;
	if ((r = git_blob_lookup(&blob, repo, &oid)) < 0)
		return -1;
	size = git_blob_rawsize(blob);
	git_blob_free(blob);
	return size;
}

	static unsigned int
l_get_file_mode(const char *path)
{
	int r;
	git_tree *tree;
	const git_tree_entry * entry;
	char last[PATH_MAX_LENGTH];
	
	if ((r = l_get_parent_tree(&tree, path)) < 0)
		return INVALID_FILE_MODE;
	if ((r = get_last_component(path, last)) < 0)
		return INVALID_FILE_MODE;
	entry = git_tree_entry_byname(tree, last);
	if (entry == NULL)
		return INVALID_FILE_MODE;

	return git_tree_entry_attributes(entry);
}

	static int
l_get_path_blob_oid(git_oid *blob_oid, const char *path)
{
	int r;

	if ((r = git_blob_create_frombuffer(blob_oid, repo, path, strlen(path)))
		< 0)
		return -1;
	return 0;
}

	static int
l_get_path_oid(git_oid *oid, const char *path)
{
	int r;
	git_tree *tree;
	const git_tree_entry *entry;
	char last[PATH_MAX_LENGTH];

	if (repo_is_dir(path)) {
		if ((r = l_get_path_tree(&tree, path)) < 0)
			return -1;
		*oid = *git_tree_id(tree);
	} else {
		if ((r = l_get_parent_tree(&tree, path)) < 0)
			return -1;
		if ((r = get_last_component(path, last)) < 0)
			return -1;
		entry = git_tree_entry_byname(tree, last);
		*oid = *git_tree_entry_id(entry);
	}
	return 0;
}

//*****************************************************************************
// STRUCTURE OF THE GIT Note for each file (git_object)
// NOTE: All links have the same result when done stat, so the full note will be
// stored only for the first note. The other notes will have the name of the
// link they are associated to. So the name of the first link.
// **Structure for first link**
// access time
// modification time
// !3 - number of links
// !/path/to/link1
// !/path/to/link2
// !/path/to/link3 
//
// **Structure for other links**
// @/path/to/first/link<NO NEW LINE> - this link will have a note associated to it which can
// be used.
//*****************************************************************************
	static int
l_update_link_stats(struct repo_stat_data *note_stat_p)
{
	int r;
	int i;
	int n;
	git_oid oid;
	git_oid path_blob_oid;
	git_signature *author;
	char *tmppath = note_stat_p->links[0];

	if (note_stat_p->count == 0 && note_stat_p->ecount == 1)
		tmppath = note_stat_p->expired_links[0];
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath)) < 0)
		return -1;
	if ((r = l_get_signature_now(&author)) < 0)
		return -1;

	DEBUG("REMOVING NOTE");
	git_note_remove(repo, NULL, author, author, &path_blob_oid);

	if (note_stat_p->count == 0)
		return 0;	// if no links, then just return

	// get the space required
	n = 0;
	n += 100; // approx for atime, mtime, number of links
	n += 4;	// for !
	n += PATH_MAX_LENGTH*note_stat_p->count;	// for paths

	// create the note for the git_object corresponding to the path
	DEBUG("CREATING NOTE DATA");
	sprintf(note_data, "%ld\n%ld\n", note_stat_p->atime, note_stat_p->mtime);
	sprintf(note_data+strlen(note_data), "!%d\n", note_stat_p->count);
	for (i=0; i<note_stat_p->count; i++) {
		sprintf(note_data+strlen(note_data), "!%s\n",
			note_stat_p->links[i]);
	}

	DEBUG("CREATING NOTE");
	if ((r = git_note_create(&oid,
				repo,
				author,
				author,
				NULL,
				&path_blob_oid,
				note_data)) < 0)
		return -1;
	DEBUG("CREATED NOTE");

	// for the other links create a linked note
	DEBUG("FINDING PATH OF LINKS");
	for (i=1; i<note_stat_p->count; i++) {
		if ((r = l_get_path_blob_oid(&path_blob_oid, note_stat_p->links[i])) < 0)
			return -1;
		DEBUG("LINK PATH %d : %s", i, note_stat_p->links[i]);
		git_note_remove(repo, NULL, author, author, &path_blob_oid);
		// create a note
		sprintf(note_data, "@%s", note_stat_p->links[0]);
		DEBUG("CREATING NOTE");
		if ((r = git_note_create(&oid,
					repo,
					author,
					author,
					NULL,
					&path_blob_oid,
					note_data)) < 0)
			return -1;
		DEBUG("NOTE CREATED");
	}
	for (i=0; i<note_stat_p->ecount; i++) {
		if ((r = l_get_path_blob_oid(&path_blob_oid, note_stat_p->expired_links[i])) < 0)
			return -1;
		DEBUG("EXPIRED LINK PATH %d : %s", i, note_stat_p->expired_links[i]);
		git_note_remove(repo, NULL, author, author, &path_blob_oid);
		DEBUG("NOTE REMOVED");
	}

	return 0;
}

/**
 * one should free the note_stat_p after its use is over
 */
	static int
l_get_note_stats(struct repo_stat_data **note_stat_p, const char *path)
{
	// TODO
	// this path has the actual note statistics
	// read the note and parse the notes
	int r;
	int i;
	git_oid path_blob_oid;
	git_note *note;
	const char *message;
	char tmp[100];
	int ind;
	char *tmp2;

	if ((r = l_get_path_blob_oid(&path_blob_oid, path)) < 0)
		return -1;

	DEBUG("READING THE NOTE");
	if ((r = git_note_read(&note, repo, NULL, &path_blob_oid)) < 0)
		return -1;
	message = git_note_message(note);
	DEBUG("NOTE MESSAGE\n%s", message);
	// ***allocate space for repo_stat_data
	DEBUG("allocate space for note_stat_p");
	*note_stat_p = malloc(sizeof(struct repo_stat_data));
	// get the atime
	DEBUG("get the atime");
	tmp2 = index(message, '\n');
	ind = tmp2 - message;
	strncpy(tmp, message, ind);
	tmp[ind] = '\0';
	sscanf(tmp, "%ld", &((*note_stat_p)->atime));
	
	message += ind+1;
	
	// get the mtime
	DEBUG("get the mtime");
	tmp2 = index(message, '\n');
	ind = tmp2 - message;
	strncpy(tmp, message, ind);
	tmp[ind] = '\0';
	sscanf(tmp, "%ld", &((*note_stat_p)->mtime));
	
	message += ind+1;
	
	// get the number of links
	DEBUG("get the number of lines");
	tmp2 = index(message, '\n');
	ind = tmp2 - message;
	strncpy(tmp, message, ind);
	tmp[ind] = '\0';
	sscanf(tmp, "!%d", &((*note_stat_p)->count));
	
	message += ind+1;

	// get the number of expired links
	DEBUG("get the expired links");
	// Note : this information is never read from the file
	(*note_stat_p)->ecount = 0;
	(*note_stat_p)->expired_links = NULL;
	
	// ***allocate space for repo_stat_data -> links
	(*note_stat_p)->links = malloc((*note_stat_p)->count * sizeof(char *));
	// ***allocate space for each repo_stat_data -> links[i]
	for (i=0; i<(*note_stat_p)->count; i++) {
		(*note_stat_p)->links[i] = malloc(PATH_MAX_LENGTH * sizeof(char));
	}

	// get the links
	DEBUG("get the links");
	for (i=0; i<(*note_stat_p)->count; i++) {
		tmp2 = index(message, '\n');
		ind = tmp2 - message;
		strncpy(tmp, message, ind);
		tmp[ind] = '\0';
		sscanf(tmp, "!%s", (*note_stat_p)->links[i]);
		
		message += ind+1;
	}

	git_note_free(note);
	return 0;
}

	static int
l_get_note_stats_link(struct repo_stat_data **note_stat_p, const char *path)
{
	int r;
	const char *message;
	git_oid path_blob_oid;
	git_note *note;
	char tmppath[PATH_MAX_LENGTH];

	if ((r = l_get_path_blob_oid(&path_blob_oid, path)) < 0)
		return -1;
	if ((r = git_note_read(&note, repo, NULL, &path_blob_oid)) < 0)
		return -1;
	
	message = git_note_message(note);
	if (message[0] == '@') {
		strcpy(tmppath, message+1);
		// free the note
		git_note_free(note);
		DEBUG("getting linked stats for %s, from %s", path, tmppath);
		if ((r = l_get_note_stats(note_stat_p, tmppath)) < 0)
			return -1;
	} else {
		// free the note
		git_note_free(note);
		DEBUG("getting original stats");
		if ((r = l_get_note_stats(note_stat_p, path)) < 0)
			return -1;
	}

	return 0;
}

	static void
l_free_str_array(char **arr, int size)
{
	int i;
	
	for (i=0; i<size; i++) {
		free(arr[i]);
	}
	free(arr);
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
	git_oid oid;
	git_treebuilder *empty_treebuilder;

	if ((r = git_repository_init(&repo, repo_address, 1)))
		return -EFG_UNKNOWN;
	// TODO test if the repository already exists, if it already exists then
	// just use it, and don't open a new repository.
	// TODO use the location specified by the config file to store the
	// repository in the parent file system

	if (git_repository_is_empty(repo) == 0)	// not empty
		return 0;
	if ((r = git_treebuilder_create(&empty_treebuilder, NULL)) < 0)
		return -EFG_UNKNOWN;	// can't create empty tree builder
	//printf("empty tree builder created\n");
	if ((r = git_treebuilder_write(&oid, repo, empty_treebuilder)) < 0)
		return -EFG_UNKNOWN;	// can't insert empty tree into repo
	//printf("tree builder inserted into repo\n");
	// free the tree builder
	git_treebuilder_free(empty_treebuilder);

	// do the commit
	char header[] = "fusegit\nsetup\n";
	//DEBUG("Making the commit : %s", message);
	if ((r = l_make_commit("/", oid, header)) < 0)
		return r;
	DEBUG("SETUP SUCCESSFUL");
	//printf("setup successful");

	return 0;
}

/**
 * checks if the path exists in the repository
 * returns 1 if the path exists, else 0
 */
	int
repo_path_exists(const char *path)
{
	int r;
	git_tree *tree;
	const git_tree_entry *entry;
	char last[PATH_MAX_LENGTH];
	
	if (strcmp(path, "/") == 0)
		return 1;
	if ((r = l_get_parent_tree(&tree, path)) < 0)
		return 0;
	if ((r = get_last_component(path, last)) < 0)
		return 0;
	entry = git_tree_entry_byname(tree, last);
	if (entry == NULL)
		return 0;
	return 1;
}

/**
 * checks if the path is of a file
 * returns 1 if it is file, else 0
 */
	int
repo_is_file(const char *path)
{
	if (!repo_path_exists(path))
		return 0;
	if (repo_is_dir(path))
		return 0;
	return 1;
}

/**
 * returns the children of the directory represented by path
 * 
 * Note : children should be freed from the function which calls it.
 */
	int
repo_get_children(struct repo_file_node **children, int *count, const char *path)
{
	int i, r, n;
	git_tree *tree;	// TODO check how to manage memory
	const git_tree_entry *entry;	// TODO check how to manage memory

	//DEBUG("ENTERING THE l_get_path_tree: \"%s\"", path);
	if ((r = l_get_path_tree(&tree, path)) < 0)
		return -EFG_UNKNOWN;
	
	n = git_tree_entrycount(tree);
	struct repo_file_node *nodes = malloc(n * sizeof(struct repo_file_node));
	*children = nodes;	// setting the children array
	*count = n;	// setting the number of children

	for (i=0; i<n; i++) {
		entry = git_tree_entry_byindex(tree, i);
		nodes[i].name = git_tree_entry_name(entry);
		//DEBUG("added.... %s", nodes[i].name);
	}
	//DEBUG("FINISHED GETTING CHILDREN");
	return 0;
}

/**
 * get the stat for a file
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
	DEBUG("GETTING ATTR OF FILE : %s", path);
	int r;
	struct repo_stat_data *file_stat;
	const struct fuse_context *ctx;
	
	if ((r = l_get_note_stats_link(&file_stat, path)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("GET STAT : %s : STAT : %x", path, stbuf);

	ctx = fuse_get_context();
	stbuf->st_dev = 1; // ignored by FUSE index_entry->dev;     /* ID of device containing file */
	stbuf->st_ino = 1; // index_entry->ino;     /* inode number */
	stbuf->st_mode = l_get_file_mode(path);    /* protection */
	if (stbuf->st_mode == INVALID_FILE_MODE)
		return -EFG_UNKNOWN;
	stbuf->st_nlink = file_stat->count;   /* number of hard links */
	stbuf->st_uid = ctx->uid;     /* user ID of owner */
	stbuf->st_gid = ctx->gid;     /* group ID of owner */
	//stbuf->st_rdev = index_entry->rdev;    /* device ID (if special file) */
	stbuf->st_size = l_get_file_size(path);    /* total size, in bytes */
	stbuf->st_blksize = 1; // ignored by FUSE index_entry->blksize; /* blocksize for file system I/O */
	//stbuf->st_blocks = index_entry->blocks;  /* number of 512B blocks allocated */
	stbuf->st_atime = file_stat->atime/1000;   /* time of last access */
	stbuf->st_mtime = file_stat->mtime/1000;   /* time of last modification */
	//stbuf->st_ctime = 0; // FIXIT   /* time of last status change */

	free_repo_stat_data(file_stat);
	
	return 0;
}

/**
 * is the path a directory
 */
	int
repo_is_dir(const char *path)
{
	int r;
	git_tree *tree;

	if (strcmp(path, "/") == 0)
		return 1;
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
	DEBUG("GETTING ATTR OF DIRECTORY : %s", path);
	const struct fuse_context *ctx;
	ctx = fuse_get_context();

	stbuf->st_dev = 1;     /* ID of device containing file */
	stbuf->st_ino = 1;     /* inode number */
	stbuf->st_mode = S_IFDIR | 0755;    /* protection */
	stbuf->st_nlink = 2;   /* number of hard links */
	stbuf->st_uid = ctx->uid;     /* user ID of owner */
	stbuf->st_gid = ctx->gid;     /* group ID of owner */
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
 * 
 * FIXIT check if the directory already exists, if it exists then throw error
 */
	int
repo_mkdir(const char *path, unsigned int attr)
{
	//DEBUG("In repo_mkdir");
	int r;
	git_treebuilder *builder;
	git_treebuilder *empty_treebuilder;
	git_tree *tree;
	git_oid oid;
	char last[PATH_MAX_LENGTH];
	char tmppath[PATH_MAX_LENGTH];
	strcpy(tmppath, path);
	
	//DEBUG("Getting parent tree");
	if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
		return -EFG_UNKNOWN;	// tmppath is incorrect
	//DEBUG("Creating tree builder");
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;	// can't get the treebuilder
	// create a empty tree
	//DEBUG("Creating empty tree builder");
	if ((r = git_treebuilder_create(&empty_treebuilder, NULL)) < 0)
		return -EFG_UNKNOWN;	// can't create empty tree builder
	//DEBUG("Writing empty tree builder to repo");
	if ((r = git_treebuilder_write(&oid, repo, empty_treebuilder)) < 0)
		return -EFG_UNKNOWN;	// can't insert empty tree into repo
	// free the tree builder
	git_treebuilder_free(empty_treebuilder);
	if ((r = get_last_component(tmppath, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Inserting into tree builder");
	if ((r = git_treebuilder_insert(NULL, builder, last, &oid, attr))
		< 0)
		return -EFG_UNKNOWN;	// can't link the empty tree to repo
	//DEBUG("Writing to the original tree");
	if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
		return -EFG_UNKNOWN;
	// free the tree builder
	git_treebuilder_free(builder);
	if ((r = get_parent_path(NULL, tmppath)) < 0)
		return -EFG_UNKNOWN;	// get parent path

	// do the commit
	char header[] = "fusegit\nmkdir\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s", header, path);
	//DEBUG("Making the commit : %s", message);
	if ((r = l_make_commit(tmppath, oid, message)) < 0)
		return r;
	//DEBUG("Commit successful");

	return 0;
}

/**
 * remove a empty directory
 */
	int
repo_rmdir(const char *path)
{
	//DEBUG("In repo_rmdir");
	int r;
	git_treebuilder *builder;
	git_tree *tree;
	git_oid oid;
	char last[PATH_MAX_LENGTH];
	char tmppath[PATH_MAX_LENGTH];
	strcpy(tmppath, path);

	// check if empty and return error if not empty
	if ((r = l_get_path_tree(&tree, tmppath)) < 0)
		return r;
	if (git_tree_entrycount(tree) > 0)
		return -EFG_NOTEMPTY;
	
	// remove the directory entry from parent, and move up the trees
	//DEBUG("Getting parent tree");
	if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
		return -EFG_UNKNOWN;	// tmppath is incorrect
	//DEBUG("Creating tree builder");
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;	// can't get the treebuilder
	if ((r = get_last_component(tmppath, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Removing the entry from the tree builder");
	if ((r = git_treebuilder_remove(builder, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Writing to the original tree");
	if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
		return -EFG_UNKNOWN;
	// free the tree builder
	git_treebuilder_free(builder);
	if ((r = get_parent_path(NULL, tmppath)) < 0)
		return -EFG_UNKNOWN;	// get parent path

	// do the commit
	char header[] = "fusegit\nrmdir\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s", header, path);
	//DEBUG("Making the commit : %s", message);
	if ((r = l_make_commit(tmppath, oid, message)) < 0)
		return r;
	//DEBUG("Commit successful");

	return 0;
}

/**
 * create a link
 */
	int
repo_link(const char *from, const char *to)
{
	//DEBUG("start : repo_link");
	// Implementation
	// In `git` every entry under any tree has a id. So we can have multiple
	// entries which are pointing to the same id. Thus we will have a link
	// to the same object in the repository, but now we have multiple links.
	int r;
	int i;
	git_treebuilder *builder;

	git_tree *from_tree;
	const git_tree_entry *from_entry;
	char from_last[PATH_MAX_LENGTH];
	unsigned int from_attr;
	
	git_tree *tree;
	git_oid oid;
	char last[PATH_MAX_LENGTH];
	char tmppath[PATH_MAX_LENGTH];
	unsigned int dir_attr = S_IFDIR | 0755;	// FIXIT HARD CODED

	struct repo_stat_data *note_stat;
	char **tmp_links;

	// find the id of the object which refers to `from`
	// return -ENOLINK if this link is not found
	if ((r = l_get_parent_tree(&from_tree, from)) < 0)
		return -EFG_NOLINK;
	if ((r = get_last_component(from, from_last)) < 0)
		return -EFG_NOLINK;
	from_entry = git_tree_entry_byname(from_tree, from_last);
	if (from_entry == NULL)
		return -EFG_NOLINK;
	oid = *git_tree_entry_id(from_entry);
	from_attr = (git_tree_entry_type(from_entry) == GIT_OBJ_BLOB) ?
		git_tree_entry_attributes(from_entry) : dir_attr;
	//DEBUG("from_attr = %o", from_attr);

	// create a new entry for `to` in the repository
	strcpy(tmppath, to);
	//DEBUG("Getting parent tree");
	if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
		return -EFG_NOLINK;	// tmppath is incorrect
	//DEBUG("Creating tree builder");
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;	// can't get the treebuilder
	if ((r = get_last_component(tmppath, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Adding the link to the tree builder");
	if ((r = git_treebuilder_insert(NULL, builder, last,
		&oid, from_attr)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Writing to the original tree");
	if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
		return -EFG_UNKNOWN;
	// free the tree builder
	git_treebuilder_free(builder);
	if ((r = get_parent_path(NULL, tmppath)) < 0)
		return -EFG_UNKNOWN;	// get parent path
	
	// do the commit
	char header[] = "fusegit\nlink\n";
	char message[2*PATH_MAX_LENGTH + strlen(" -> ") + strlen(header)];
	sprintf(message, "%s%s -> %s", header, from, to);
	//DEBUG("Making the commit : %s", message);
	if ((r = l_make_commit(tmppath, oid, message)) < 0)
		return r;
	//DEBUG("Commit successful");

	// create the link stats
	if ((r = l_get_note_stats_link(&note_stat, from)) < 0)
		return -EFG_UNKNOWN;
	note_stat->count += 1;
	tmp_links = malloc(note_stat->count * sizeof(char *));
	for (i=0; i<note_stat->count-1; i++) {
		tmp_links[i] = malloc(PATH_MAX_LENGTH * sizeof(char));
		strcpy(tmp_links[i], note_stat->links[i]);
	}
	tmp_links[i] = malloc(PATH_MAX_LENGTH * sizeof(char));
	strcpy(tmp_links[i], to);
	free(note_stat->links);
	note_stat->links = tmp_links;
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -EFG_UNKNOWN;
	free_repo_stat_data(note_stat);

	return 0;
}

/**
 * unlink a path
 */
	int
repo_unlink(const char *path)
{
	//DEBUG("In repo_unlink");
	int r;
	int i, j, k;
	git_treebuilder *builder;
	const git_tree_entry *entry;
	git_tree *tree;
	git_oid oid;
	char last[PATH_MAX_LENGTH];
	char tmppath[PATH_MAX_LENGTH];
	strcpy(tmppath, path);
	
	struct repo_stat_data *note_stat;
	char **tmp_links = NULL;
	char **tmp_exp_links = NULL;

	// update the link stats
	if ((r = l_get_note_stats_link(&note_stat, path)) < 0)
		return -EFG_UNKNOWN;
	note_stat->count -= 1;
	note_stat->ecount = 1;
	tmp_links = malloc(note_stat->count * sizeof(char *));
	tmp_exp_links = malloc(note_stat->ecount * sizeof(char *));
	for (i=0, j=0, k=0; i<note_stat->count+1; i++) {
		DEBUG("COMPARE : %s : %s", note_stat->links[i], path);
		if (strcmp(note_stat->links[i], path) == 0) {
			tmp_exp_links[k] = malloc(PATH_MAX_LENGTH * sizeof(char));
			strcpy(tmp_exp_links[k], note_stat->links[i]);
			k++;
			DEBUG("CHECKING EXP LINK WAS WRITTEN");
			continue;
		}
		tmp_links[j] = malloc(PATH_MAX_LENGTH * sizeof(char));
		strcpy(tmp_links[j], note_stat->links[i]);
		j++;
	}
	if (note_stat->expired_links)
		l_free_str_array(note_stat->expired_links, note_stat->ecount);
	note_stat->expired_links = tmp_exp_links;
	l_free_str_array(note_stat->links, note_stat->count);
	note_stat->links = tmp_links;
	DEBUG("CHECKING EXP LINK : %s", note_stat->expired_links[0]);
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("FREEING MEMORY IN UNLINK for note_stat");
	free_repo_stat_data(note_stat);
	DEBUG("FREED MEMORY IN UNLINK for note_stat");

	// check if it is a file
	if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
		return r;
	if ((r = get_last_component(tmppath, last)) < 0)
		return -EFG_UNKNOWN;
	entry = git_tree_entry_byname(tree, last);
	if (entry == NULL)
		return -EFG_UNKNOWN;
	if (git_tree_entry_type(entry) != GIT_OBJ_BLOB)
		return -EFG_NOLINK;
	

	// TODO modularize the code
	// below is a modular code to do the above stuff.
	// 1. make the changes required for this function
	// check if it is a file
	if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
		return r;
	if ((r = get_last_component(tmppath, last)) < 0)
		return -EFG_UNKNOWN;
	entry = git_tree_entry_byname(tree, last);
	if (entry == NULL)
		return -EFG_UNKNOWN;
	if (git_tree_entry_type(entry) != GIT_OBJ_BLOB)
		return -EFG_NOLINK;
	
	// remove the directory entry from parent, and move up the trees
	//DEBUG("Getting parent tree");
	if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
		return -EFG_UNKNOWN;	// tmppath is incorrect
	//DEBUG("Creating tree builder");
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;	// can't get the treebuilder
	if ((r = get_last_component(tmppath, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Removing the entry from the tree builder");
	if ((r = git_treebuilder_remove(builder, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Writing to the original tree");
	if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
		return -EFG_UNKNOWN;
	// free the tree builder
	git_treebuilder_free(builder);
	if ((r = get_parent_path(NULL, tmppath)) < 0)
		return -EFG_UNKNOWN;	// get parent path
	
	// 2. update the tree above you, till the head.
	// do the commit
	char header[] = "fusegit\nunlink\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s", header, path);
	//DEBUG("Making the commit : %s", message);
	if ((r = l_make_commit(tmppath, oid, message)) < 0)
		return r;
	return 0;
}


/**
 * read a file
 */
	int
repo_read(const char *path, char *buf, size_t size, off_t offset)
{

	// get the blob
	// we get the blob 
	git_blob *blob;
	int r;
	char last[PATH_MAX_LENGTH];
	git_tree *tree;
	const git_tree_entry *entry;
	git_oid oid;
	char *content;
	size_t rawsize;
	// to get the parent tree we use the l_get_parent_tree.
	DEBUG("get the parent tree");
	if ((r = l_get_parent_tree(&tree, path)) < 0) 
		return -EFG_UNKNOWN;

	// to get the filename we have the path.
	DEBUG("get the last component");
	if ((r = get_last_component(path, last)) < 0)
		return -EFG_UNKNOWN;
	// to get the entry we need the filename and parent tree
	DEBUG("get the tree entry by name");
	entry = git_tree_entry_byname(tree, last);
	// to get the entry id , we need the entry
	DEBUG("get the tree entry id");
	oid = *git_tree_entry_id(entry);
	// git_blob_lookup but for this we need entry ID. 
	DEBUG("get the blob from tree entry id");
	if ((r = git_blob_lookup(&blob, repo, &oid)) < 0)
		return -EFG_UNKNOWN;
		
	// once we get the blob, read it and store it in the buf
	DEBUG("get the blob rawcontent and size");
	content = (char *) git_blob_rawcontent(blob);
	rawsize = git_blob_rawsize(blob);

	// if size is greater than rawsize, return rawsize
	if ((size + offset) > rawsize)
		(size = rawsize - offset) > 0 ? size : (size = 0) ;
	
	DEBUG("copy the content to the fuse buffer");
	memcpy(buf, content, size);

	git_blob_free(blob);
	
	// return the size of the content we have read
	return size;
	
}

/**
 * create a empty file
 */
	int
repo_create_file(const char *path, mode_t mode)
{
	int r;
	git_oid oid;
	git_tree *tree;
	git_treebuilder	*builder;
	char tmppath[PATH_MAX_LENGTH];
	char last[PATH_MAX_LENGTH];
	strcpy(tmppath, path);
	struct repo_stat_data *note_stat;

	// create a empty blob
	if ((r = git_blob_create_frombuffer(&oid, repo, "", 0)) < 0)
		return -EFG_UNKNOWN;

	// get the parent tree
	if ((r = l_get_parent_tree(&tree, path)) < 0)
		return -EFG_UNKNOWN;

	// get the treebuilder for the parent tree
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;

	// add the blob with the name of the file as a child to the parent
	if ((r = get_last_component(path, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Adding the link to the tree builder");
	if ((r = git_treebuilder_insert(NULL, builder, last,
		&oid, (unsigned int)mode)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Writing to the original tree");
	if ((r = git_treebuilder_write(&oid, repo, builder)) < 0)
		return -EFG_UNKNOWN;
	// free the tree builder
	git_treebuilder_free(builder);
	if ((r = get_parent_path(NULL, tmppath)) < 0)
		return -EFG_UNKNOWN;	// get parent path
	
	// do the commit
	char header[] = "fusegit\ncreate\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s", header, path);
	if ((r = l_make_commit(tmppath, oid, message)) < 0)
		return r;

	// create the link stats
	note_stat = malloc(sizeof(struct repo_stat_data));
	note_stat->atime = 0;
	note_stat->mtime = 0;
	note_stat->count = 1;
	note_stat->links = malloc(sizeof(char *));
	note_stat->links[0] = malloc(PATH_MAX_LENGTH * sizeof(char));
	strcpy(note_stat->links[0], path);
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -1;
	free_repo_stat_data(note_stat);

	return 0;
}

/**
 * update the access and modification times of a file
 */
	int
repo_update_time_ns(const char *path, const struct timespec ts[2])
{
	int r;
	struct repo_stat_data *note_stat;

	// get the note for the git_object corresponding to the path
	if ((r = l_get_note_stats_link(&note_stat, path)) < 0)
		return -EFG_UNKNOWN;

	// read the note_stat and update all the links
	note_stat->atime = ts[0].tv_sec*1000;
	note_stat->mtime = ts[1].tv_sec*1000;
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -EFG_UNKNOWN;
	free_repo_stat_data(note_stat);
	return 0;
}

/**
 * truncate a file
 */
	int
repo_truncate(const char *path, off_t size)
{
	int r;
	git_oid oid;
	git_oid tree_oid;
	git_blob *blob;
	git_tree *tree;
	const git_tree_entry *entry;
	git_treebuilder *builder;
	char buf[size];
	char *content;
	unsigned int mode;
	char tmppath[PATH_MAX_LENGTH];
	char last[PATH_MAX_LENGTH];
	strcpy(tmppath, path);

	// get the blob id corresponding to this path
	if ((r = l_get_path_oid(&oid, path)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_blob_lookup(&blob, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	// check if the size is greater than blob size, then just return
	if (git_blob_rawsize(blob) <= size)
		return 0;	// Done here, return

	// create a new blob with the required size
	content = (char *)git_blob_rawcontent(blob);
	memcpy(buf, content, size);
	git_blob_free(blob);
	if ((r = git_blob_create_frombuffer(&oid, repo, buf, size)) < 0)
		return -EFG_UNKNOWN;

	// get the parent tree
	if ((r = l_get_parent_tree(&tree, path)) < 0)
		return -EFG_UNKNOWN;

	// get the treebuilder for the parent tree
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;

	// add the blob with the name of the file as a child to the parent
	if ((r = get_last_component(path, last)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Adding the link to the tree builder");
	entry = git_tree_entry_byname(tree, last);
	mode = git_tree_entry_attributes(entry);
	if ((r = git_treebuilder_insert(NULL, builder, last,
		&oid, mode)) < 0)
		return -EFG_UNKNOWN;
	//DEBUG("Writing to the original tree");
	if ((r = git_treebuilder_write(&tree_oid, repo, builder)) < 0)
		return -EFG_UNKNOWN;
	// free the tree builder
	git_treebuilder_free(builder);
	if ((r = get_parent_path(NULL, tmppath)) < 0)
		return -EFG_UNKNOWN;	// get parent path
	
	// call make commit function with this updated blob id
	// do the commit
	char header[] = "fusegit\ntruncate\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s : %ld", header, path, size);
	//DEBUG("Making the commit : %s", message);
	if ((r = l_make_commit(tmppath, tree_oid, message)) < 0)
		return r;
	//DEBUG("Commit successful");

	return 0;
}

	void
free_repo_stat_data(struct repo_stat_data *data)
{
	int i;
	
	DEBUG("FREEING INDIVIDUAL LINKS");
	for (i=0; i<data->count; i++) {
		free(data->links[i]);
	}
	DEBUG("FREEING INDIVIDUAL EXPIRED LINKS");
	for (i=0; i<data->ecount; i++) {
		free(data->expired_links[i]);
	}
	DEBUG("FREEING LINKS");
	if (data->count)
		free(data->links);	// TODO check if this is correct
	DEBUG("FREEING EXPIRED LINKS");
	if (data->ecount)
		free(data->expired_links);
	DEBUG("FREEING DATA");	// TODO check if this is correct
	free(data);
}

/**
 * write to a file
 * 
 * FIXIT : currently the file system keeps the data to be written in memroy and
 * writes it to the file subsequently. But this will fail for large files.
 */
	int
repo_write(const char *path, const char *buf, size_t size, off_t offset)
{
	int r;
	int i;
	git_oid oid;
	git_oid tree_oid;
	git_blob *blob;
	git_tree *tree;
	const git_tree_entry *entry;
	git_treebuilder *builder;
	char write_buf[size+offset];
	char *blob_content;
	unsigned int mode;
	//char tmppath[PATH_MAX_LENGTH];
	char *tmppath;
	char last[PATH_MAX_LENGTH];
	struct repo_stat_data *stat_data;
	//strcpy(tmppath, path);

	// get the blob corresponding to this path
	if ((r = l_get_path_oid(&oid, path)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_blob_lookup(&blob, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	if (offset == 0) {
	// if the offset is 0
	// create a blob with the buf and size, and make path to point to it
		memcpy(write_buf, buf, size);
	} else {
	// if the offset is not 0
	// obtain the blob for the path, create a new blob with the appended
	// content and make path to point to it
		blob_content = (char *)git_blob_rawcontent(blob);
		memcpy(write_buf, blob_content, offset);
		memcpy(write_buf+offset, buf, size);
	}
	git_blob_free(blob);

	if ((r = l_get_note_stats_link(&stat_data, path)) < 0)
		return -EFG_UNKNOWN;

	// create a new blob with the required size
	if ((r = git_blob_create_frombuffer(&oid, repo, write_buf, offset+size)) < 0)
		return -EFG_UNKNOWN;

	for (i=0; i<stat_data->count; i++) {
		tmppath = stat_data->links[i];
		// get the parent tree
		if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
			return -EFG_UNKNOWN;

		// get the treebuilder for the parent tree
		if ((r = git_treebuilder_create(&builder, tree)) < 0)
			return -EFG_UNKNOWN;

		// add the blob with the name of the file as a child to the parent
		if ((r = get_last_component(tmppath, last)) < 0)
			return -EFG_UNKNOWN;
		//DEBUG("Adding the link to the tree builder");
		entry = git_tree_entry_byname(tree, last);
		mode = git_tree_entry_attributes(entry);
		if ((r = git_treebuilder_insert(NULL, builder, last,
			&oid, mode)) < 0)
			return -EFG_UNKNOWN;
		//DEBUG("Writing to the original tree");
		if ((r = git_treebuilder_write(&tree_oid, repo, builder)) < 0)
			return -EFG_UNKNOWN;
		// free the tree builder
		git_treebuilder_free(builder);
		if ((r = get_parent_path(NULL, tmppath)) < 0)
			return -EFG_UNKNOWN;	// get parent path
		
		// call make commit function with this updated blob id
		// do the commit
		char header[] = "fusegit\nwrite\n";
		char message[PATH_MAX_LENGTH + strlen(header) + 10];
		sprintf(message, "%slink: %s : %ld", header, tmppath, size);
		//DEBUG("Making the commit : %s", message);
		if ((r = l_make_commit(tmppath, tree_oid, message)) < 0)
			return r;
		//DEBUG("Commit successful");
	}

	return size;
}
