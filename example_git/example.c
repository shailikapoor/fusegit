/**
 * a test file to see how to use libgit2
 */


#include <git2.h>
#include <stdio.h>
#include <time.h>

git_time_t get_time(void);
int create_repo(const char *repo_name);
int read_repo(const char *repo_name);

int
main(int argc, char *argv[])
{
	create_repo("repo.git");
	read_repo("repo.git");
	//edit_repo("repo.git");
	//read_repo("repo.git");
	return 0;
}

git_time_t
get_time(void)
{
	return 100;
}

int
create_repo(const char *repo_name)
{
	int r;

	git_repository *repo;
	git_oid blob_id;
	git_oid oid;
	git_oid tree_id;
	git_signature *author;
	git_time_t time;
	git_config *config;
	git_index *index;
	git_tree *tree;
	git_treebuilder *tree_builder;
	git_treebuilder *empty_tree_builder;
	char global_config_path[GIT_PATH_MAX];
	
	// create the repository
	r = git_repository_init(&repo, repo_name, 1);
	if (r)
		printf("error in creating repository\n");
	printf("Repo created\n");
	
	// set the repository config
	git_config_new(&config);
	git_config_find_global(global_config_path, GIT_PATH_MAX);
	git_config_add_file_ondisk(config, global_config_path, 1);
	//git_config_set_string(config, "name", "Varun Agrawal");
	//git_config_set_string(config, "email", "varun729@gmail.com");
	git_repository_set_config(repo, config);
	printf("Repo config set\n");

	// create a treebuilder
	r = git_treebuilder_create(&tree_builder, NULL);
	if (r)
		printf("error in creting treebuilder\n");
	printf("Tree builder created\n");

	// ADDING FIRST FILE
	// create a blob
	r = git_blob_create_fromdisk(&blob_id, repo, "test1");
	if (r)
		printf("error in creating blob from disk\n");
	printf("Blob created\n");

	// insert into tree
	r = git_treebuilder_insert(NULL, tree_builder, "test1", &blob_id, 0100644);
	if (r)
		printf("error in inserting into treebuilder\n");
	printf("Insert into treebuilder successful\n");

	// ADDING SECOND FILE
	// create a blob
	r = git_blob_create_fromdisk(&blob_id, repo, "test2");
	if (r)
		printf("error in creating blob from disk\n");
	printf("Blob created\n");

	// insert into tree
	r = git_treebuilder_insert(NULL, tree_builder, "test2", &blob_id, 0100644);
	if (r)
		printf("error in inserting into treebuilder\n");
	printf("Insert into treebuilder successful\n");

	// ADDING A EMPTY FOLDER
	// create a empty tree
	r = git_treebuilder_create(&empty_tree_builder, NULL);
	if (r)
		printf("error in creting empty treebuilder\n");
	printf("Empty Tree builder created\n");
	
	// write the empty tree to the repo
	r = git_treebuilder_write(&tree_id, repo, empty_tree_builder);
	if (r)
		printf("error in writing the empty tree to the repo\n");
	printf("Writing the empty tree to repo successful\n");


	// insert empty tree into the tree
	r = git_treebuilder_insert(NULL, tree_builder, "test_dir", &tree_id, 0040000);
	if (r)
		printf("error in inserting into treebuilder\n");
	printf("Insert into treebuilder successful\n");

	// write the tree to the repo
	r = git_treebuilder_write(&oid, repo, tree_builder);
	if (r)
		printf("error in writing the tree to the repo\n");
	printf("Writing the tree to repo successful\n");

	// tree lookup
	r = git_tree_lookup(&tree, repo, &oid);
	if (r)
		printf("error in tree lookup\n");
	printf("Tree lookup done\n");
	
	// create a author
	time = get_time();
	r = git_signature_new(&author, "Varun Agrawal", "varun729@gmail.com", time, -300);
	if (r)
		printf("error in creating signature\n");
	printf("Author signature created\n");

	// create a commit
	r = git_commit_create(  &oid, // object id
				repo, // repository
				"HEAD", // update reference, this will update the HEAD to this commit
				author, // author
				author, // committer
				NULL, // message encoding, by default UTF-8 is used
				"first commit", // message for the commit
				tree, // the git_tree object which will be used as the tree for this commit. don't know if NULL is valid
				0, // number of parents. Don't know what value should be used here
				NULL); // array of pointers to the parents(git_commit *parents[])
	if (r)
		printf("error in creating a commit\n");
	printf("Commit created\n");
	
	git_repository_free(repo);
	return 0;
}

int
read_repo(const char *repo_name)
{
	int i;
	int r;
	int n;
	git_repository *repo;
	git_reference *head;
	git_oid oid;
	git_commit *commit;
	git_tree *tree;
	git_tree_entry *tree_entry;
	char out[41];
	out[40] = '\0';

	// opening the repository
	r = git_repository_open(&repo, repo_name);
	if (r)
		printf("error in opening the repository\n");
	printf("Opened the repository successfully.\n");

	// obtaining the head
	r = git_repository_head(&head, repo);
	if (r)
		printf("error in obtaining the head\n");
	r = git_reference_name_to_oid(&oid, repo, git_reference_name(head));
	if (r)
		printf("error in obtaining the ref id of head\n");
	printf("Obtained the head id %s\n", git_oid_tostr(out, 41, &oid));

	// obtaining the commit from commit id
	r = git_commit_lookup(&commit, repo, &oid);
	if (r)
		printf("error in obtaining the commit from oid\n");
	printf("Obtained the commit.\n");

	// obtaining the tree id from the commit
	oid = *git_commit_tree_oid(commit);

	// get the tree
	r = git_tree_lookup(&tree, repo, &oid);
	if (r)
		printf("error in looking up the tree for oid\n");
	printf("Lookup for tree of oid successful.\n");

	n = git_tree_entrycount(tree);
	for (i=0; i<n; i++) {
		tree_entry = git_tree_entry_byindex(tree, i);
		printf("entry >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s %s\n", git_tree_entry_name(tree_entry), git_object_type2string(git_tree_entry_type(tree_entry)));
	}

	git_repository_free(repo);
	return 0;
}

int
edit_repo(const char *repo_name)
{
	int r;

	git_repository *repo;
	git_oid blob_id;
	git_oid oid;
	git_oid tree_id;
	git_signature *author;
	git_time_t time;
	git_config *config;
	git_index *index;
	git_tree *tree;
	git_treebuilder *tree_builder;
	git_treebuilder *empty_tree_builder;
	git_reference *head;
	git_commit *commit_parents[1];
	char global_config_path[GIT_PATH_MAX];
	char out[41];
	out[40] = '\0';
	
	// create the repository
	r = git_repository_open(&repo, repo_name);
	if (r)
		printf("error in opening repository\n");
	printf("Repo opened\n");
	
	// create a treebuilder
	r = git_treebuilder_create(&tree_builder, NULL);
	if (r)
		printf("error in creting treebuilder\n");
	printf("Tree builder created\n");

	// ADDING FIRST FILE
	// create a blob
	r = git_blob_create_fromdisk(&blob_id, repo, "test2");
	if (r)
		printf("error in creating blob from disk\n");
	printf("Blob created\n");

	// insert into tree
	r = git_treebuilder_insert(NULL, tree_builder, "test1", &blob_id, 0100644);
	if (r)
		printf("error in inserting into treebuilder\n");
	printf("Insert into treebuilder successful\n");
	
	// write the tree to the repo
	r = git_treebuilder_write(&oid, repo, tree_builder);
	if (r)
		printf("error in writing the tree to the repo\n");
	printf("Writing the tree to repo successful\n");

	// tree lookup
	r = git_tree_lookup(&tree, repo, &oid);
	if (r)
		printf("error in tree lookup\n");
	printf("Tree lookup done\n");
	
	// create a author
	time = get_time();
	r = git_signature_new(&author, "Varun Agrawal", "varun729@gmail.com", time, -300);
	if (r)
		printf("error in creating signature\n");
	printf("Author signature created\n");

	// obtaining the head
	r = git_repository_head(&head, repo);
	if (r)
		printf("error in obtaining the head\n");
	r = git_reference_name_to_oid(&oid, repo, git_reference_name(head));
	if (r)
		printf("error in obtaining the ref id of head\n");
	printf("Obtained the head id %s\n", git_oid_tostr(out, 41, &oid));
	git_commit_lookup(&commit_parents[0], repo, &oid);

	// create a commit
	r = git_commit_create(  &oid, // object id
				repo, // repository
				"HEAD", // update reference, this will update the HEAD to this commit
				author, // author
				author, // committer
				NULL, // message encoding, by default UTF-8 is used
				"second commit", // message for the commit
				tree, // the git_tree object which will be used as the tree for this commit. don't know if NULL is valid
				1, // number of parents. Don't know what value should be used here
				commit_parents); // array of pointers to the parents(git_commit *parents[])
	if (r)
		printf("error in creating a commit\n");
	printf("Commit created\n");
	
	git_repository_free(repo);
	return 0;
}

