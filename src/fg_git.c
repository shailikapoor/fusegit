/**
 * fg_git.c
 */
#include <git2.h>
#include <assert.h>
#include <time.h>
#include "fg.h"
#include "fg_repo.h"
#include "fg_util.h"

// ****************************************************************************
// ALL NEW FILE STRUCTURE
// The git repository main tree will not be the '/' directory anymore. It will
// be called 'root'. It will have 2 children, and possibly more in future. One
// will be the '/' root directory. The other child will be 'inodes'.
// 
// '/' child will remain almost the same, although some code changes will be
// required.
// 
// 'inodes' will be a radix tree kind of structure, which will have several
// levels. I'll keep the number of levels as '5', but it will remain a variable
// parameter, and not hardcoded. At each level a node, can have at most 64
// children. So 5 levels means 64^5 leaves. Which is the maximum number of
// inodes supported in the file system right now. Which is already too much,
// because libgit2 may not be able to handle all of them.
// ****************************************************************************



// LOCAL
static char REF_NAME[PATH_MAX_LENGTH] = "refs/heads/master";
static const unsigned int INVALID_FILE_MODE = 077777777;
static git_repository *repo;
static git_commit *last_commit = NULL;
char note_data[PATH_MAX_LENGTH*(MAX_HARD_LINKS+1)];
struct repo_file_handle_queue file_queue = {.next = 0};
pthread_mutex_t note_lock;

struct copy_ref {
	char *from;
	char *to;
};

	void
l_init_repo_stat_data(struct repo_stat_data **note_stat)
{
	time_t now;
	const struct fuse_context *ctx;

	ctx = fuse_get_context();
	time(&now);
	DEBUG("CURRENT TIME IS : %ld", now);
	*note_stat = malloc(sizeof(struct repo_stat_data));
	(*note_stat)->atime = now*1000;
	(*note_stat)->mtime = now*1000;
	(*note_stat)->uid = ctx->uid;
	(*note_stat)->gid = ctx->gid;
	(*note_stat)->count = 1;
	(*note_stat)->links = NULL;
	(*note_stat)->ecount = 0;
	(*note_stat)->expired_links = NULL;
}

	static int
l_get_last_commit(git_commit **commit_p)
{
	int r;
	git_oid oid;
	//git_reference *ref;
	const char *ref_name;
	// obtaining the head
	//DEBUG("OBTAINING THE HEAD");
	//if ((r = git_repository_head(&ref, repo)) < 0)
	//	return -EFG_UNKNOWN;

	// obtaining the commit id from the reference
	//DEBUG("OBTAINING commit id from the reference");
	//ref_name = git_reference_name(ref);
	ref_name = REF_NAME;
	if ((r = git_reference_name_to_oid(&oid, repo, ref_name))
		< 0)
		return -EFG_UNKNOWN;
	DEBUG("got the oid for the last commit");
	//git_reference_free(ref);

	// obtaining the commit from the commit id
	//DEBUG("OBTAINING commit from the commit id");
	if ((r = git_commit_lookup(commit_p, repo, &oid)) < 0)
		return -EFG_UNKNOWN;
	
	return 0;
}

// OBSOLETE
// Should be modified to look for the root at a lower level.
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

// OBSOLETE
// This code should be parameterized, shouldn't hardcode the user and email.
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

/**
 * @param path path which was changed and has to be committed
 * @param oid the object id of the tree which is at path
 * @param message message of the commit
 */
	static int
l_make_commit(const char *path, git_oid oid, const char *message)
{
	int r;
	git_treebuilder *builder;
	const git_tree_entry *entry;
	git_tree *tree;
	unsigned int attr;
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
		entry = git_tree_entry_byname(tree, last);
		attr = git_tree_entry_attributes(entry);
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

// OBSOLETE
// This shouldn't be computed. This value should be read from the inode of the
// file. For this, the inode should be updated with the new value whenever we do
// write or truncate. Also, it should first check if the file handle is in the
// buffer. If yes, then the information of the last blob, will be taken from the
// buffer.
// NOTE: What if the file was full till the last blob. The next write will
// create a new blob for the buffer, so that should not be mistaken as the last
// blob. Figure out a way to handle this case.
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

// OBSOLETE
// Read the file mode from the inode
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

// OBSOLETE : remove this
	static int
l_get_path_blob_oid(git_oid *blob_oid, const char *buffer)
{
	int r;

	if ((r = git_blob_create_frombuffer(blob_oid, repo, buffer,
		strlen(buffer)))
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
// STRUCTURE OF THE INODE for each file/directory
// NOTE: All the files and directories will be stored as trees, and the
// information of whether they are directory or file is stored in the first
// entry, which we will call "inode:0"
// **Structure of inode:0**
// access time
// modification time
// $uid
// $gid
// $mode <----- this is NEW
// $size <----- this is NEW
// !3 - number of links
// !/path/to/link1
// !/path/to/link2
// !/path/to/link3 
//
// **Structure for other links**
// @/path/to/first/link<NO NEW LINE> - this link will have a note associated to it which can
// be used.
//*****************************************************************************

// OBSOLETE : remove this
	static void
l_get_note_name(char *out_name, const char *in_name, const char *prefix)
{
	out_name[0] = '\0';
	strcat(out_name, prefix);
	strcat(out_name, ":");
	strcat(out_name, in_name);
}

// OBSOLETE : remove this
	static int
l_remove_link_stats(const char *path)
{
	int r;
	git_oid path_blob_oid;
	git_signature *author;
	char tmppath_ref_name[PATH_MAX_LENGTH];

	l_get_note_name(tmppath_ref_name, path, REF_NAME);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
		return -1;
	if ((r = l_get_signature_now(&author)) < 0)
		return -1;
	if ((r = git_note_remove(repo, NULL, author, author, &path_blob_oid)) <
		0)
		return -1;
	return 0;
}

// OBSOLETE : remove this
	static int
l_update_link_stats(const struct repo_stat_data *note_stat_p)
{
	DEBUG("***l_update_link_stats LOCKED");
	pthread_mutex_lock(&note_lock);
	int r;
	int i;
	int n;
	git_oid oid;
	git_oid path_blob_oid;
	git_signature *author;
	char *tmppath = note_stat_p->links[0];
	char tmppath_ref_name[PATH_MAX_LENGTH];

	DEBUG("COUNT = %d", note_stat_p->count);
	DEBUG("ECOUNT = %d", note_stat_p->ecount);
	if (note_stat_p->count == 0 && note_stat_p->ecount == 1)
		tmppath = note_stat_p->expired_links[0];
	l_get_note_name(tmppath_ref_name, tmppath, REF_NAME);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
		goto error;//return -1;
	if ((r = l_get_signature_now(&author)) < 0)
		goto error;//return -1;

	DEBUG("SILENTLY REMOVING NOTE(error not thrown on error)");
	git_note_remove(repo, NULL, author, author, &path_blob_oid);

	if (note_stat_p->count == 0)
		goto success;//return 0;	// if no links, then just return

	// get the space required
	n = 0;
	n += 100; // approx for atime, mtime, number of links
	n += 4;	// for !
	n += PATH_MAX_LENGTH*note_stat_p->count;	// for paths

	// create the note for the git_object corresponding to the path
	DEBUG("CREATING NOTE DATA");
	sprintf(note_data, "%ld\n%ld\n", note_stat_p->atime, note_stat_p->mtime);
	sprintf(note_data+strlen(note_data), "$%d\n", note_stat_p->uid);
	sprintf(note_data+strlen(note_data), "$%d\n", note_stat_p->gid);
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
		goto error;//return -1;
	DEBUG("CREATED NOTE");

	// for the other links create a linked note
	DEBUG("FINDING PATH OF LINKS");
	for (i=1; i<note_stat_p->count; i++) {
		l_get_note_name(tmppath_ref_name, note_stat_p->links[i],
		REF_NAME);
		if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
			goto error;//return -1;
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
			goto error;//return -1;
		DEBUG("NOTE CREATED");
	}
	assert(note_stat_p->ecount == 0 || note_stat_p->ecount == 1);
	for (i=0; i<note_stat_p->ecount; i++) {
		l_get_note_name(tmppath_ref_name, note_stat_p->expired_links[i],
			REF_NAME);
		if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
			goto error;//return -1;
		DEBUG("EXPIRED LINK PATH %d : %s", i, note_stat_p->expired_links[i]);
		git_note_remove(repo, NULL, author, author, &path_blob_oid);
		DEBUG("NOTE REMOVED");
	}

	git_signature_free(author);
success:
	DEBUG("***l_update_link_stats UNLOCKED");
	pthread_mutex_unlock(&note_lock);
	return 0;
error:
	DEBUG("***l_update_link_stats UNLOCKED");
	pthread_mutex_unlock(&note_lock);
	return -1;
}

/**
 * one should free the note_stat_p after its use is over
 */
// OBSOLETE : remove this
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
	char tmppath_ref_name[PATH_MAX_LENGTH];

	l_get_note_name(tmppath_ref_name, path, REF_NAME);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
		return -1;

	DEBUG("READING THE NOTE");
	if ((r = git_note_read(&note, repo, NULL, &path_blob_oid)) < 0)
		return -1;
	message = git_note_message(note);
	DEBUG("NOTE MESSAGE\n%s", message);
	// ***allocate space for repo_stat_data
	DEBUG("allocate space for note_stat_p");
	l_init_repo_stat_data(note_stat_p);
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
	
	// get the uid
	DEBUG("get the uid");
	tmp2 = index(message, '\n');
	ind = tmp2 - message;
	strncpy(tmp, message, ind);
	tmp[ind] = '\0';
	sscanf(tmp, "$%d", &((*note_stat_p)->uid));
	
	message += ind+1;
	
	// get the gid
	DEBUG("get the gid");
	tmp2 = index(message, '\n');
	ind = tmp2 - message;
	strncpy(tmp, message, ind);
	tmp[ind] = '\0';
	sscanf(tmp, "$%d", &((*note_stat_p)->gid));
	
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

// OBSOLETE : remove this
	static int
l_get_note_stats_link(struct repo_stat_data **note_stat_p, const char *path)
{
	DEBUG("** l_get_note_stats_link %s", path);
	int r;
	const char *message;
	git_oid path_blob_oid;
	git_note *note;
	char tmppath[PATH_MAX_LENGTH];
	char tmppath_ref_name[PATH_MAX_LENGTH];

	l_get_note_name(tmppath_ref_name, path, REF_NAME);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
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
// FILE HANDLE

	static int
l_create_file_handle(uint64_t *fh, const char *path)
{
	int err = -1;
	int next;
	//DEBUG("***l_create_file_handle LOCKED");
	//pthread_mutex_lock(&file_queue.lock);
	next = file_queue.next;
	DEBUG("searching for the next file handle");
	if (file_queue.handles[next] == NULL) {
		file_queue.handles[next] = malloc(sizeof(struct repo_file_handle));
		file_queue.handles[next]->path = NULL;
	} else if (file_queue.handles[next]->free) {
		// use this
	} else {
		while (next != file_queue.next) {
			DEBUG("next = %d", next);
			next = (next+1) % MAX_HANDLES;
			if (file_queue.handles[next] == NULL) {
				file_queue.handles[next] = malloc(sizeof(struct repo_file_handle));
				file_queue.handles[next]->path = NULL;
			} else if (file_queue.handles[next]->free) {
				// use this
			} else {
				next++;
				continue;
			}
			break;
		}
		if (next == file_queue.next) {	// queue is full
			err = -EFG_MAX_HANDLES;
			goto error;//return -EFG_MAX_HANDLES;
		}
	}
	DEBUG("found the next file handle: %d", next);
	struct repo_file_handle *handle = file_queue.handles[next];
	//DEBUG("handle %d LOCKED", next);
	//pthread_mutex_lock(&handle->lock);
	// set the file handle as non-free
	handle->free = 0;
	// set the file path
	if (handle->path != NULL)
		free(handle->path);
	handle->path = malloc(sizeof(char) * (strlen(path)+1));
	strcpy(handle->path, path);
	DEBUG("set the file handle");
	// set the file handle
	*fh = next;
	file_queue.next = (next + 1) % MAX_HANDLES;
	// set the size and off to 0
	handle->size = 0;
	handle->off = 0;
	// mark the handle as non-dirty
	handle->dirty = 0;
	DEBUG("finished creating file handle");
//success:
	//DEBUG("handle UNLOCKED");
	//DEBUG("***l_create_file_handle UNLOCKED");
	//pthread_mutex_unlock(&handle->lock);
	//pthread_mutex_unlock(&file_queue.lock);
	return 0;
error:
	//DEBUG("handle UNLOCKED");
	//DEBUG("***l_create_file_handle UNLOCKED");
	//pthread_mutex_unlock(&handle->lock);
	//pthread_mutex_unlock(&file_queue.lock);
	return err;

}

	static int
l_get_file_handle(struct repo_file_handle **handle, uint64_t fh)
{
	if (fh >= MAX_HANDLES)
		return -1;
	*handle = file_queue.handles[(int)fh];
	if (!(*handle)->free)
		return 0;
	*handle = NULL;
	return -1;
}

	static int
l_release_file_handle(const char *path, uint64_t fh)
{
	int err = -1;
	//DEBUG("***l_release_file_handle LOCKED");
	//pthread_mutex_lock(&file_queue.lock);
	struct repo_file_handle *handle = file_queue.handles[(int)fh];
	//DEBUG("handle %d LOCKED", fh);
	//pthread_mutex_lock(&handle->lock);
	if (strcmp(handle->path, path) != 0)
		goto error;//return -1;
	handle->free = 1;

//success:
	//DEBUG("handle UNLOCKED");
	//DEBUG("***l_release_file_handle UNLOCKED");
	//pthread_mutex_unlock(&handle->lock);
	//pthread_mutex_unlock(&file_queue.lock);
	return 0;
error:
	//DEBUG("handle UNLOCKED");
	//DEBUG("***l_release_file_handle UNLOCKED");
	//pthread_mutex_unlock(&handle->lock);
	//pthread_mutex_unlock(&file_queue.lock);
	return err;
}



/**
 * this function just writes the content to the disk. It doesn't clear the
 * buffer or sets the size and off of the handle to 0. This should be done by
 * the calling function
 */
// OBSOLETE
// instead of the earlier write function, write in chunks of 4096 bytes only,
// and create individual blobs for each chunk and add it to the proper inode
// tree
	static int
l_write_to_disk(struct repo_file_handle *handle, const char *operation)
{
	int err = -1;
	//DEBUG("handle LOCKED: %s", operation);
	//pthread_mutex_lock(&handle->lock);
	const char *path = handle->path;
	const char *buf = handle->buf;
	size_t size = handle->size;
	off_t offset = handle->off;
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
	char *tmppath;
	char last[PATH_MAX_LENGTH];
	struct repo_stat_data *stat_data;

	// get the blob corresponding to this path
	if ((r = l_get_path_oid(&oid, path)) < 0)
		goto error;//return -EFG_UNKNOWN;
	if ((r = git_blob_lookup(&blob, repo, &oid)) < 0)
		goto error;//return -EFG_UNKNOWN;

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
		goto error;//return -EFG_UNKNOWN;

	// create a new blob with the required size
	if ((r = git_blob_create_frombuffer(&oid, repo, write_buf, offset+size)) < 0)
		goto error;//return -EFG_UNKNOWN;

	for (i=0; i<stat_data->count; i++) {
		tmppath = stat_data->links[i];
		// get the parent tree
		if ((r = l_get_parent_tree(&tree, tmppath)) < 0)
			goto error;//return -EFG_UNKNOWN;

		// get the treebuilder for the parent tree
		if ((r = git_treebuilder_create(&builder, tree)) < 0)
			goto error;//return -EFG_UNKNOWN;

		// add the blob with the name of the file as a child to the parent
		if ((r = get_last_component(tmppath, last)) < 0)
			goto error;//return -EFG_UNKNOWN;
		//DEBUG("Adding the link to the tree builder");
		entry = git_tree_entry_byname(tree, last);
		mode = git_tree_entry_attributes(entry);
		if ((r = git_treebuilder_insert(NULL, builder, last,
			&oid, mode)) < 0)
			goto error;//return -EFG_UNKNOWN;
		//DEBUG("Writing to the original tree");
		if ((r = git_treebuilder_write(&tree_oid, repo, builder)) < 0)
			goto error;//return -EFG_UNKNOWN;
		// free the tree builder
		git_treebuilder_free(builder);
		if ((r = get_parent_path(NULL, tmppath)) < 0)
			goto error;//return -EFG_UNKNOWN;	// get parent path
		
		// call make commit function with this updated blob id
		// do the commit
		char header[] = "fusegit\nl_write_to_disk\n";
		char message[PATH_MAX_LENGTH + strlen(header) + 10];
		sprintf(message, "%s%s\n%s", header, operation, path);
		//DEBUG("Making the commit : %s", message);
		if ((r = l_make_commit(tmppath, tree_oid, message)) < 0)
			goto error;//return r;
		//DEBUG("Commit successful");
	}

//success:
	//DEBUG("handle UNLOCKED");
	//pthread_mutex_unlock(&handle->lock);
	return size;
error:
	//DEBUG("handle UNLOCKED");
	//pthread_mutex_unlock(&handle->lock);
	return err;
}

// OBSOLETE
// Read the data from the file from its different chunks, and then copy it to
// the buffer. Right now all data is in one big blob, but now you might have to
// read several blobs to get the whole data.
	static int
l_copy_data_to_buffer(char *buf, struct repo_file_handle *handle, size_t size,
	off_t off)
{
	if (size > handle->size)
		return -1;
	if (off < handle->off)
		return -1;
	if ((off + size) > (handle->off + handle->size))
		return -1;
	memcpy(buf, handle->buf+(off-handle->off), size);
	return 0;
}

	static int
l_copy_data_from_buffer(const char *buf, struct repo_file_handle *handle, size_t size,
	off_t off)
{
	if (size > FILE_BUFFER_CHUNK)
		return -1;
	if ((off%FILE_BUFFER_CHUNK + size) > FILE_BUFFER_CHUNK)
		return -1;
	if ((off - off%FILE_BUFFER_CHUNK) != handle->off) {
		l_write_to_disk(handle, "l_copy_data_from_buffer:writing more data");
		handle->off = off - off%FILE_BUFFER_CHUNK;
		handle->size = 0;
		handle->dirty = 0;
	}
	// handle->off is set before this step
	// need to fill handle->size, buf, dirty
	if (size > 0) {
		DEBUG("******* writing in memory");
		memcpy(handle->buf + (off-handle->off), buf, size);
		handle->size = off + size - handle->off;
		handle->dirty = 1;
	}
	return 0;
}

// ****************************************************************************
// GLOBAL

/**
 * Setup the Git repository at the address repo_address
 */
// OBSOLETE
// This creates the home directory and stores its metadata in the notes, this
// should be replaced by the inode:0 structure, in the inodes tree.
	int
repo_setup(const char *repo_address)
{
	int r;
	int i;
	git_oid oid;
	git_treebuilder *empty_treebuilder;
	struct repo_stat_data *note_stat;

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
	
	// create the link stats
	l_init_repo_stat_data(&note_stat);
	note_stat->links = malloc(1 * sizeof(char *));
	note_stat->links[0] = malloc(PATH_MAX_LENGTH * sizeof(char));
	strcpy(note_stat->links[0], "/");
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -EFG_UNKNOWN;
	free_repo_stat_data(note_stat);

	// setup the file_queue
	for (i=0; i<MAX_HANDLES; i++) {
		file_queue.handles[i] = NULL;
	}

	return 0;
}

/**
 * checks if the path exists in the repository
 * returns 1 if the path exists, else 0
 */
// OBSOLETE
// See if the handle for the file already exists.
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
// OBSOLETE
// To identify whether a tree is a file or a directory, read its inode, and the
// read the first entry of the inode, which is inode:0, and figure out the type
// of file it is. It may be a symbolic link, in which case do something special
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
 * is the path a directory
 */
// OBSOLETE
// To identify whether a tree is a file or a directory, read its inode, and the
// read the first entry of the inode, which is inode:0, and figure out the type
// of file it is. It may be a symbolic link, in which case do something special
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
 * returns the children of the directory represented by path
 * 
 * Note : children should be freed from the function which calls it.
 */
// OBSOLETE
// In the for loop you have entry index from 0 to n, it should now be 1 to n.
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
// OBSOLETE
// Use the inode and then look for inode:0 entry in that inode.
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
	
	if ((r = l_get_note_stats_link(&file_stat, path)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("obtained the note stats");

	stbuf->st_dev = 1; // ignored by FUSE index_entry->dev;     /* ID of device containing file */
	stbuf->st_ino = 1; // index_entry->ino;     /* inode number */
	stbuf->st_mode = l_get_file_mode(path);    /* protection */
	if (stbuf->st_mode == INVALID_FILE_MODE)
		return -EFG_UNKNOWN;
	stbuf->st_nlink = file_stat->count;   /* number of hard links */
	stbuf->st_uid = file_stat->uid;     /* user ID of owner */
	stbuf->st_gid = file_stat->gid;     /* group ID of owner */
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
 * get the stat for a directory
 */
// OBSOLETE
// Use the new inode:0 blob in the inode to read the data instead of notes.
	int
repo_dir_stat(const char *path, struct stat *stbuf)
{
	DEBUG("GETTING ATTR OF DIRECTORY : %s", path);
	int r;
	struct repo_stat_data *file_stat;

	if ((r = l_get_note_stats_link(&file_stat, path)) < 0)
		return -EFG_UNKNOWN;

	stbuf->st_dev = 1;     /* ID of device containing file */
	stbuf->st_ino = 1;     /* inode number */
	stbuf->st_mode = l_get_file_mode(path);    /* protection */
	if (stbuf->st_mode == INVALID_FILE_MODE)
		stbuf->st_mode = S_IFDIR | 0755;  // FIXIT  /* protection */
	stbuf->st_nlink = 2;   /* number of hard links */
	stbuf->st_uid = file_stat->uid;     /* user ID of owner */
	stbuf->st_gid = file_stat->gid;     /* group ID of owner */
	//stbuf->st_rdev = index_entry->rdev;    /* device ID (if special file) */
	stbuf->st_size = 0;    /* total size, in bytes */
	//stbuf->st_blksize = index_entry->blksize; /* blocksize for file system I/O */
	//stbuf->st_blocks = index_entry->blocks;  /* number of 512B blocks allocated */
	stbuf->st_atime = file_stat->atime/1000;   /* time of last access */
	stbuf->st_mtime = file_stat->mtime/1000;   /* time of last modification */
	//stbuf->st_ctime = 0;   /* time of last status change */
	
	return 0;
}

/**
 * make a directory in the given path
 * 
 */
// OBSOLETE
// mkdir should create tree, with one blob as a child, which is a blob with the
// inode number. This inode in the inode tree, should have the metadata file
// inode:0.
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
	struct repo_stat_data *note_stat;
	
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

	// create the link stats
	l_init_repo_stat_data(&note_stat);
	note_stat->links = malloc(1 * sizeof(char *));
	note_stat->links[0] = malloc(PATH_MAX_LENGTH * sizeof(char));
	strcpy(note_stat->links[0], path);
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -EFG_UNKNOWN;
	free_repo_stat_data(note_stat);

	return 0;
}

/**
 * remove a empty directory
 */
// OBSOLETE
// remove the tree entry correspondig to this directory. Also the inode for this
// directory should be removed.
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

	// remove the link stats
	if ((r = l_remove_link_stats(path)) < 0)
		return -EFG_UNKNOWN;

	return 0;
}

/**
 * create a link
 */
// OBSOLETE
// Take the tree corresponding to 'from'. That will contain a blob with the
// inode number. Obtain the inode corresponding to that inode number, and
// increse the link count to contain this new file. No need to store the path to
// the new file. Create a new tree entry for 'to' with a blob containing the
// same inode number.
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
	from_attr = git_tree_entry_attributes(from_entry);
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
// OBSOLETE
// Obtain the blob corresponding to blob, and decrease the links count in the
// inode:0 blob. Now remove the link to path in the tree structure.
// If the links count in the inode:0 become 0, remove the inode also. Removing
// the inode code should be a helper function, because this will also be called
// when the file is closed.
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
// OBSOLETE
// Knowing the offset and the size, we know which blobs we need to read in the
// file. Obtain the tree corresponding to the file. Get the blobs corresponding
// to offset:size. And then read and put it in buf.
// A lot of read code is written here. This should all be moved to the helper
// function.
	int
repo_read(const char *path, char *buf, size_t size, off_t offset, uint64_t fh)
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
	struct repo_file_handle *handle;

	if ((r = l_get_file_handle(&handle, fh)) < 0)
		return -EFG_UNKNOWN;
	// l_copy_data_to_buffer returns 0 on success
	if (l_copy_data_to_buffer(buf, handle, size, offset) == 0) {
		return size;
	}
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

	// copy contents to the file handle
	handle->size = (offset + size) % FILE_BUFFER_CHUNK;
	handle->off = (offset + size) - handle->size;
	DEBUG("copying to file handle: offset=%d, size=%d", handle->off,
		handle->size);
	memcpy(handle->buf, content+handle->off, handle->size);

	git_blob_free(blob);
	
	// return the size of the content we have read
	return size;
	
}

/**
 * create a empty file
 */
// OBSOLETE
// Creating a file, just need to create a tree with one blob, which contains the
// inode. Find the next available free inode, and use it. If there is no free
// inode, then you cannot create a new file. The inode, should have a blob
// called inode:0 to contain the metadata, and blobs 1:62 will contain data.
// Instead of a blob 63, there will be another tree if the file is that big.
// This will again have a maximum of 64 children, and if required the 63rd child
// will again be a tree. So we can have huge files.
	int
repo_create_file(const char *path, mode_t mode, uint64_t *fh)
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
	l_init_repo_stat_data(&note_stat);
	note_stat->links = malloc(1 * sizeof(char *));
	note_stat->links[0] = malloc(PATH_MAX_LENGTH * sizeof(char));
	strcpy(note_stat->links[0], path);
	if ((r = l_update_link_stats(note_stat)) < 0)
		return -EFG_UNKNOWN;
	free_repo_stat_data(note_stat);
	
	// create the file handle
	if ((r = l_create_file_handle(fh, path)) < 0)
		return r;

	return 0;
}

/**
 * update the access and modification times of a file
 */
// OBSOLETE
// Just need to update the inode:0 blob for the inode of the file. This will
// change the tree id, and so the new tree needs to be linked to all the links
// in the inode:0.
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
// OBSOLETE
// Truncate the file, and update the inode:0 of the files inode
	int
repo_truncate(const char *path, off_t size)
{
	int r;
	int i;
	git_oid oid;
	git_oid tree_oid;
	git_blob *blob;
	git_tree *tree;
	const git_tree_entry *entry;
	git_treebuilder *builder;
	char buf[size];
	char *blob_content;
	unsigned int mode;
	char *tmppath;
	char last[PATH_MAX_LENGTH];
	struct repo_stat_data *stat_data;

	// get the blob id corresponding to this path
	if ((r = l_get_path_oid(&oid, path)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_blob_lookup(&blob, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	// check if the size is greater than blob size, then just return
	if (git_blob_rawsize(blob) <= size)
		return 0;	// Done here, return

	// create a new blob with the required size
	blob_content = (char *)git_blob_rawcontent(blob);
	memcpy(buf, blob_content, size);
	git_blob_free(blob);

	if ((r = l_get_note_stats_link(&stat_data, path)) < 0)
		return -EFG_UNKNOWN;

	if ((r = git_blob_create_frombuffer(&oid, repo, buf, size)) < 0)
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
		char header[] = "fusegit\ntruncate\n";
		char message[PATH_MAX_LENGTH + strlen(header) + 10];
		sprintf(message, "%slink: %s : %ld", header, tmppath, size);
		//DEBUG("Making the commit : %s", message);
		if ((r = l_make_commit(tmppath, tree_oid, message)) < 0)
			return r;
		//DEBUG("Commit successful");
	}

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
// OBSOLETE
// this function writes to the file, so if we have abstracted all the code in
// the helper function, similar to repo_read, then nothing much to be done here.
	int
repo_write(const char *path, const char *buf, size_t size, off_t offset, uint64_t fh)
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
	char *tmppath;
	char last[PATH_MAX_LENGTH];
	struct repo_stat_data *stat_data;
	struct repo_file_handle *handle;

	if ((r = l_get_file_handle(&handle, fh)) < 0)
		return -EFG_UNKNOWN;
	// returns 0 on successful copy to buffer
	if ((r = l_copy_data_from_buffer(buf, handle, size, offset)) == 0)
		return size;

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
		sprintf(message, "%s%s", header, path);
		//DEBUG("Making the commit : %s", message);
		if ((r = l_make_commit(tmppath, tree_oid, message)) < 0)
			return r;
		//DEBUG("Commit successful");
	}

	return size;
}

/**
 * rename a file
 */
// OBSOLETE
// Just need to create a different link. The inodes are all handled, and so no
// more handling will be required. Also, if someone had opened the file
// previously, and then renamed the file - read and write will still work
// normally.
	int
repo_rename_file(const char *from, const char *to)
{
	int r;

	if ((r = repo_link(from, to)) < 0)
		return -EFG_UNKNOWN;
	if ((r = repo_unlink(from)) < 0)
		return -EFG_UNKNOWN;
	return 0;
}

	static int
l_callback_rename_dir_cleanup(const char *root, git_tree_entry *entry, void
	*payload)
{
	int r;
	struct copy_ref *copy_data = payload; // from:from parent, to:to parent
	char name[PATH_MAX_LENGTH] = "";
	strcpy(name, copy_data->from);
	strcat(name, "/");
	strcat(name, root);
	strcat(name, git_tree_entry_name(entry));
	// remove the original note
	if ((r = l_remove_link_stats(name)) < 0)
		return -1;
	return 0;
}

	static int
l_callback_rename_dir(const char *root, git_tree_entry *entry, void *payload)
{	
	int r;
	int i;
	struct copy_ref *copy_data = payload; // from:from parent, to:to parent
	char from_name[PATH_MAX_LENGTH] = "";
	char to_name[PATH_MAX_LENGTH] = "";
	struct repo_stat_data *note_stat_p;
	strcpy(from_name, copy_data->from);
	if (strcmp(copy_data->from, "/") != 0)
		strcat(from_name, "/");
	strcat(from_name, root);
	strcat(from_name, git_tree_entry_name(entry));
	strcpy(to_name, copy_data->to);
	if (strcmp(copy_data->to, "/") != 0)
		strcat(to_name, "/");
	strcat(to_name, root);
	strcat(to_name, git_tree_entry_name(entry));

	// get the note from the repository
	/*
	l_get_note_name(tmppath_ref_name, from_name, REF_NAME);
	if ((r = l_get_path_blob_oid(&from_path_blob_oid, tmppath_ref_name)) < 0)
		return -1;
	if ((r = git_note_read(&note, repo, NULL, &from_path_blob_oid)) < 0)
		return -1;
	message = git_note_message(note);
	*/
	// NOTE: if for some file, you don't get the notes, then it has been
	// fixed
	// TODO : do proper testing for this feature, when you have links
	DEBUG("copying the notes from %s to %s", from_name, to_name);
	if ((r = l_get_note_stats_link(&note_stat_p, from_name)) < 0)
		return -1;

	for (i=0; i<note_stat_p->count; i++) {
		if (strcmp(note_stat_p->links[i], from_name) == 0) {
			strcpy(note_stat_p->links[i], to_name);
			break;
		}
	}
	if ((r = l_update_link_stats(note_stat_p)) < 0)
		return -1;
	DEBUG("copying successful");

	return 0;
}

/**
 * rename a directory
 */
// OBSOLETE
// Just need to create a different link. The inodes are all handled, and so no
// more handling will be required. Also, if someone had opened the file
// previously, and then renamed the file - read and write will still work
// normally.
	int
repo_rename_dir(const char *from, const char *to)
{
	// TODO
	// TODO ALARM! This is going to be very bad
	// TODO Renaming the folder will almost be the same task, just you don't need
	// TODO to worry about the links, since linking is not supported for the
	// directories.
	// TODO BUT, what about the files in the directory. Since the path for the
	// TODO directories is now changed, and their attributes are saved for a note
	// TODO for a different path. All the attributes have to be reassigned, and
	// TODO you need to write the attributes for the file all over again. So a
	// TODO different data structure needs to be designed for the attributes.
	// TODO The above issue is not fixed and the below code is a bad
	// workaround.
	int r;
	int i;
	git_oid oid;
	git_tree *tree;
	const git_tree_entry *from_entry;
	git_tree *from_parent_tree;
	git_tree *to_parent_tree;
	git_treebuilder *from_parent_builder;
	git_treebuilder *to_parent_builder;
	char last[PATH_MAX_LENGTH];
	char from_copy[PATH_MAX_LENGTH];
	char to_copy[PATH_MAX_LENGTH];
	char from_parent[PATH_MAX_LENGTH];
	char to_parent[PATH_MAX_LENGTH];
	char header[100];
	char message[3*PATH_MAX_LENGTH];
	unsigned int dir_attr;
	struct copy_ref copy_data;
	struct repo_stat_data *note_stat_p;

	strcpy(from_copy, from);
	strcpy(to_copy, to);

	// get the parent tree of from, then get its tree builder.
	if ((r = get_parent_path(from, from_parent)) < 0)
		return -EFG_UNKNOWN;
	if ((r = l_get_parent_tree(&from_parent_tree, from)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_treebuilder_create(&from_parent_builder, from_parent_tree)) <
		0)
		return -EFG_UNKNOWN;
	if ((r = get_last_component(from, last)) < 0)
		return -EFG_UNKNOWN;
	from_entry = git_tree_entry_byname(from_parent_tree, last);
	dir_attr = git_tree_entry_attributes(from_entry);
	// get the tree entry of from.
	if ((r = l_get_path_tree(&tree, from)) < 0)
		return -EFG_UNKNOWN;
	oid = *git_tree_id(tree);

	// get the parent tree of to, then get its tree builder.
	if ((r = get_parent_path(to, to_parent)) < 0)
		return -EFG_UNKNOWN;
	if ((r = l_get_parent_tree(&to_parent_tree, to)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_treebuilder_create(&to_parent_builder, to_parent_tree)) <
		0)
		return -EFG_UNKNOWN;

	// add the from treeentry to parent of to. Commit this change as the
	// first step of rename_dir
	if ((r = get_last_component(to, last)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_treebuilder_insert(NULL, to_parent_builder, last, &oid,
		dir_attr))
		< 0)
		return -EFG_UNKNOWN;
	if ((r = git_treebuilder_write(&oid, repo, to_parent_builder)) < 0)
		return -EFG_UNKNOWN;
	// do the commit
	strcpy(header, "fusegit\nrename_dir Step1(copy the contents)\n");
	sprintf(message, "%s%s -> %s", header, from, to);
	if ((r = l_make_commit(to_parent, oid, message)) < 0)
		return -EFG_UNKNOWN;

	// free the tree builder
	git_treebuilder_free(to_parent_builder);

	// remove the tree entry of from, from this tree builder.
	if ((r = get_last_component(from, last)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_treebuilder_remove(from_parent_builder, last)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_treebuilder_write(&oid, repo, from_parent_builder)) < 0)
		return -EFG_UNKNOWN;

	// commit this change as the second step of rename_dir
	// do the commit
	strcpy(header, "fusegit\nrename_dir Step2(remove the fromm)\n");
	sprintf(message, "%s%s -> %s", header, from, to);
	if ((r = l_make_commit(from_parent, oid, message)) < 0)
		return -EFG_UNKNOWN;

	// free the tree builder
	git_treebuilder_free(from_parent_builder);

	// copying the notes
	// pass the parent paths to the callback function, and modify the notes
	// for each entry in the tree which is moved.
	DEBUG("tree callback");
	// NOTE : PRE-ORDER tree traversal is not implemented in libgit2
	copy_data.from = from_copy; //from_parent;
	copy_data.to = to_copy; //to_parent;
	if ((r = git_tree_walk(tree, l_callback_rename_dir, GIT_TREEWALK_POST,
		&copy_data)) < 0) {
		DEBUG("error in callback");
		return -EFG_UNKNOWN;
	}
	if ((r = git_tree_walk(tree, l_callback_rename_dir_cleanup, GIT_TREEWALK_POST,
		&copy_data)) < 0) {
		DEBUG("error in callback");
		return -EFG_UNKNOWN;
	}

	// copy the directories note attributes
	if ((r = l_get_note_stats_link(&note_stat_p, from)) < 0)
		return -1;

	for (i=0; i<note_stat_p->count; i++) {
		if (strcmp(note_stat_p->links[i], from) == 0) {
			strcpy(note_stat_p->links[i], to);
			break;
		}
	}
	if ((r = l_update_link_stats(note_stat_p)) < 0)
		return -1;
	if ((r = l_remove_link_stats(from)) < 0)
		return -1;

	return 0;
}

// OBSOLETE
	static int
l_callback_copy_note_attr(const char *root, git_tree_entry *entry, void
	*payload)
{
	int r;
	struct copy_ref *copy_data = payload;
	git_oid oid;
	git_oid path_blob_oid;
	git_note *note;
	git_signature *author;
	char *tag_name = copy_data->to;
	char *ref_name = copy_data->from;
	char name[PATH_MAX_LENGTH] = "";
	char tmppath_ref_name[PATH_MAX_LENGTH];
	const char *message;
	strcat(name, "/");
	strcat(name, root);
	strcat(name, git_tree_entry_name(entry));

	DEBUG("root = %s, entry = %s, payload = %s, from = %s, to = %s", root,
		name, tag_name, copy_data->from, copy_data->to);
	// get the note from the repository
	l_get_note_name(tmppath_ref_name, name, ref_name);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
		return -1;
	if ((r = git_note_read(&note, repo, NULL, &path_blob_oid)) < 0)
		return -1;
	message = git_note_message(note);

	// create a note for the tag
	l_get_note_name(tmppath_ref_name, name, tag_name);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name)) < 0)
		return -1;
	if ((r = l_get_signature_now(&author)) < 0)
		return -1;
	if ((r = git_note_create(&oid,
				repo,
				author,
				author,
				NULL,
				&path_blob_oid,
				message)) < 0)
		return -1;

	git_note_free(note);
	git_signature_free(author);
	DEBUG("root = %s, entry = %s, payload = %s", root, name, tag_name);
	return 0;
}

/**
 * take a backup
 *
 * TODO Right now the REF_NAME is hardcoded. But when taking backup this has to be
 * obtained from a config file. So read it.
 */
// OBSOLETE
// This doesn't require to update notes. It is now much simpler.
	int
repo_backup(const char *snapshot)
{
	int r;
	git_oid oid;
	git_commit *commit_p;
	git_signature *author;
	git_tag *tag;
	git_tree *tree;
	git_object *object;
	const char *tag_name;
	char tag_name_copy[PATH_MAX_LENGTH];
	char message[100];
	struct copy_ref copy_data;
	char tmppath_ref_name[PATH_MAX_LENGTH];
	git_oid path_blob_oid;
	git_note *note;
	const char *note_message;
	
	// get the latest commit of the repository
	if ((r = l_get_last_commit(&commit_p)) < 0)
		return -EFG_UNKNOWN;

	// create the tag
	if ((r = l_get_signature_now(&author)) < 0)
		return -EFG_UNKNOWN;
	sprintf(message, "fusegit\nBACKUP\n%s", snapshot);
	
	oid = *git_commit_id(commit_p);
	if ((r = git_object_lookup(&object, repo, &oid, GIT_OBJ_ANY)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_tag_create(&oid, repo, snapshot, object, author, message,
		0)) < 0)
		return -EFG_UNKNOWN;
	
	if ((r = git_tag_lookup(&tag, repo, &oid)) < 0)
		return -EFG_UNKNOWN;
	tag_name = git_tag_name(tag);
	strcpy(tag_name_copy, tag_name);
	DEBUG("BACKUP TAG NAME:%s", tag_name);
	git_tag_free(tag);

	// recursively get each of the elements in this tag and store a
	// copy of the note for each of the files

	// get the header tree
	if ((r = l_get_last_commit(&commit_p)) < 0)
		return -EFG_UNKNOWN;
	oid = *git_commit_id(commit_p);
	
	oid = *git_commit_tree_oid(commit_p);

	if ((r = git_tree_lookup(&tree, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	// do a tree walk using libgit2 and then copy the note for each
	// element in the tree
	DEBUG("tree callback");
	// NOTE : PRE-ORDER tree traversal is not implemented in libgit2
	copy_data.from = REF_NAME;
	copy_data.to = tag_name_copy;
	if ((r = git_tree_walk(tree, l_callback_copy_note_attr, GIT_TREEWALK_POST,
		&copy_data)) < 0) {
		DEBUG("error in callback");
		return -EFG_UNKNOWN;
	}

	l_get_note_name(tmppath_ref_name, "/", REF_NAME);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name))
		< 0)
		return -EFG_UNKNOWN;
	if ((r = git_note_read(&note, repo, NULL, &path_blob_oid)) < 0)
		return -EFG_UNKNOWN;
	note_message = git_note_message(note);
	
	// create a note for the tag
	l_get_note_name(tmppath_ref_name, "/", tag_name_copy);
	if ((r = l_get_path_blob_oid(&path_blob_oid, tmppath_ref_name))
		< 0)
		return -EFG_UNKNOWN;
	if ((r = l_get_signature_now(&author)) < 0)
		return -EFG_UNKNOWN;
	if ((r = git_note_create(&oid,
				repo,
				author,
				author,
				NULL,
				&path_blob_oid,
				note_message)) < 0)
		return -EFG_UNKNOWN;
	
	git_note_free(note);
	git_signature_free(author);
	DEBUG("repo_backup successful");
	
	// free the repository
	git_commit_free(commit_p);
	git_repository_free(repo);
	return 0;
}

/**
 * restore the repository
 */
// OBSOLETE
// This doesn't require to update notes. It is now much simpler.
	int
repo_restore(const char *snapshot)
{
	int r;
	int i;

	// testing if HEAD can be changed
	git_reference *ref;
	git_reference *tag_ref;
	git_oid oid;
	git_strarray array;
	git_tag *tag;
	git_object *new_head_commit_object;
	git_commit *commit_p;
	git_tree *tree;
	char tag_name[PATH_MAX_LENGTH];
	char idstr[PATH_MAX_LENGTH];
	struct copy_ref copy_data;

	strcpy(tag_name, "refs/tags/");
	strcat(tag_name, snapshot);

	if ((r = git_reference_list(&array, repo, GIT_REF_LISTALL)) < 0)
		return -EFG_UNKNOWN;
	for (i=0; i<array.count; i++) {
		DEBUG("REFERENCE %d: %s", i, array.strings[i]);
	}

	if ((r = git_repository_head(&ref, repo)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("obtained the head");
	DEBUG("looking for tag: %s", tag_name);
	if ((r = git_reference_lookup(&tag_ref, repo, tag_name)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("obtained the tag");
	oid = *git_reference_oid(tag_ref);

	if ((r = git_tag_lookup(&tag, repo, &oid)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("tag type: %d (GIT_OBJ_BAD=%d)", git_tag_type(tag), GIT_OBJ_BAD);

	if ((r = git_tag_peel(&new_head_commit_object, tag)) < 0)
		return -EFG_UNKNOWN;
	oid = *git_object_id(new_head_commit_object);
	git_oid_tostr(idstr, GIT_OID_HEXSZ+1, &oid);
	DEBUG("commit id: %s", idstr);
	if ((r = git_reference_set_oid(ref, &oid)) < 0)
		return -EFG_UNKNOWN;
	//if ((r = git_reference_reload(ref)) < 0)
	//	return -EFG_UNKNOWN;

	// update the notes
	DEBUG("updating the notes");
	// get the header tree
	if ((r = l_get_last_commit(&commit_p)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("obtained the last commit");
	//oid = *git_commit_id(commit_p);
	
	oid = *git_commit_tree_oid(commit_p);

	if ((r = git_tree_lookup(&tree, repo, &oid)) < 0)
		return -EFG_UNKNOWN;

	// do a tree walk using libgit2 and then copy the note for each
	// element in the tree
	DEBUG("tree callback");
	strcpy(tag_name, git_tag_name(tag));
	DEBUG("TAG NAME FOR NOTES COPY: %s", tag_name);
	// NOTE : PRE-ORDER tree traversal is not implemented in libgit2
	copy_data.from = tag_name;
	copy_data.to = REF_NAME;
	if ((r = git_tree_walk(tree, l_callback_copy_note_attr, GIT_TREEWALK_POST,
		&copy_data)) < 0) {
		DEBUG("error in callback");
		return -EFG_UNKNOWN;
	}
	DEBUG("branch changed");
	return 0;
}

/**
 * change file permissions
 */
// OBSOLETE
// This doesn't require to update notes. It just needs to find the inode number
// and update that inode.
	int
repo_chmod(const char *path, mode_t mode)
{
	int r;
	git_oid oid;
	git_tree *tree;
	git_treebuilder	*builder;
	const git_tree_entry *entry;
	char tmppath[PATH_MAX_LENGTH];
	char last[PATH_MAX_LENGTH];
	strcpy(tmppath, path);

	// get the parent tree
	if ((r = l_get_parent_tree(&tree, path)) < 0)
		return -EFG_UNKNOWN;

	// get the treebuilder for the parent tree
	if ((r = git_treebuilder_create(&builder, tree)) < 0)
		return -EFG_UNKNOWN;

	// add the blob with the name of the file as a child to the parent
	if ((r = get_last_component(path, last)) < 0)
		return -EFG_UNKNOWN;

	entry = git_tree_entry_byname(tree, last);
	oid = *git_tree_entry_id(entry);
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
	char header[] = "fusegit\nchmod\n";
	char message[PATH_MAX_LENGTH + strlen(header)];
	sprintf(message, "%s%s", header, path);
	if ((r = l_make_commit(tmppath, oid, message)) < 0)
		return r;

	return 0;
}

/**
 * change owner
 */
// OBSOLETE
// This doesn't require to update notes. It just needs to find the inode number
// and update that inode.
	int
repo_chown(const char *path, uid_t uid, gid_t gid)
{
	int r;
	struct repo_stat_data *stat_data;

	if ((r = l_get_note_stats_link(&stat_data, path)) < 0)
		return -EFG_UNKNOWN;
	stat_data->uid = uid;
	stat_data->gid = gid;
	if ((r = l_update_link_stats(stat_data)) < 0)
		return -EFG_UNKNOWN;
	return 0;
}

/**
 * open a file
 */
// OBSOLETE
// When you open a file handle for the file, increase the links count in the
// inode by 1.
	int
repo_open(const char *path, uint64_t *fh)
{
	int r;

	if ((r = l_create_file_handle(fh, path)) < 0)
		return r;
	return 0;
}

/**
 * release a file
 */
// OBSOLETE
// When you close a file handle for the file, decrease the links count in the
// inode by 1. If the links count becomes 0, then remove the file.
	int
repo_release(const char *path, uint64_t fh)
{
	int r;
	struct repo_file_handle *handle;

	if ((r = l_get_file_handle(&handle, fh)) < 0)
		return -EFG_UNKNOWN;
	if (handle->dirty && ((r = repo_flush(path, fh)) < 0))
		return -EFG_UNKNOWN;
	if ((r = l_release_file_handle(path, fh)) < 0)
		return -EFG_UNKNOWN;
	return 0;
}

/**
 * flush a file
 */
	int
repo_flush(const char *path, uint64_t fh)
{
	int r;
	struct repo_file_handle *handle;

	DEBUG("getting file handle");
	if ((r = l_get_file_handle(&handle, fh)) < 0)
		return -EFG_UNKNOWN;
	DEBUG("comparing path with the file handle");
	if (strcmp(handle->path, path) != 0)
		return -EFG_UNKNOWN;
	// if not dirty
	DEBUG("checking if handle is dirty");
	if (handle->dirty == 0)
		return 0;
	DEBUG("writing to disk");
	if ((r = l_write_to_disk(handle, "flush")) < 0)
		return -EFG_UNKNOWN;
	DEBUG("flush successful");
	handle->size = 0;
	handle->off = 0;
	handle->dirty = 0;
	
	return 0;
}

/**
 * fsync a file
 * 
 * TODO right now implemented exactly like flush
 */
	int
repo_fsync(const char *path, uint64_t fh, int isdatasync)
{
	int r;
	struct repo_file_handle *handle;

	if ((r = l_get_file_handle(&handle, fh)) < 0)
		return -EFG_UNKNOWN;
	if (strcmp(handle->path, path) != 0)
		return -EFG_UNKNOWN;
	// if not dirty
	if (handle->dirty == 0)
		return 0;
	if ((r = l_write_to_disk(handle, "fsync")) < 0)
		return -EFG_UNKNOWN;
	handle->size = 0;
	handle->off = 0;
	handle->dirty = 0;
	
	return 0;
}

