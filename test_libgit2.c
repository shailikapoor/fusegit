/**
 * a test file to see how to use libgit2
 */


#include <git2.h>
#include <stdio.h>
#include <time.h>



git_time_t
get_time(void)
{
	return 100;
}

int
main(int argc, char *argv[])
{
	int r;

	git_repository *repo;
	git_oid blob_id;
	git_oid oid;
	git_signature *author;
	git_time_t time;
	git_config *config;
	git_index *index;
	git_tree *tree;
	git_treebuilder *tree_builder;
	char global_config_path[GIT_PATH_MAX];
	
	// create the repository
	r = git_repository_init(&repo, "repo.git", 1);
	if (r < 0)
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

	// create a blob
	r = git_blob_create_fromdisk(&blob_id, repo, "/home/varun/work/gitfs/gitfs.c");
	if (r < 0)
		printf("error in creating blob from disk\n");
	printf("Blob created\n");
	
	// create a author
	time = get_time();
	r = git_signature_new(&author, "Varun Agrawal", "varun729@gmail.com", time, -300);
	if (r < 0)
		printf("error in creating signature\n");
	printf("Author signature created\n");

	// BELOW THIS NOT SURE
	// create a index
	r = git_index_new(&index);
	if (r < 0)
		printf("error in creating new index\n");
	printf("New index created in memory\n");

	// create a treebuilder
	r = git_treebuilder_create(&tree_builder, NULL);
	if (r < 0)
		printf("error in creting treebuilder\n");
	printf("Tree builder created\n");

	// build a tree
	r = git_treebuilder_insert(NULL, tree_builder, "gitfs.c", &oid, (git_filemode_t)0100644);

	// create a commit
	r = git_commit_create(  &oid, // object id
				repo, // repository
				"HEAD", // update reference, this will update the HEAD to this commit
				author, // author
				author, // committer
				NULL, // message encoding, by default UTF-8 is used
				"temp message", // message for the commit
				tree, // the git_tree object which will be used as the tree for this commit. don't know if NULL is valid
				0, // number of parents. Don't know what value should be used here
				NULL); // array of pointers to the parents(git_commit *parents[])
	if (r < 0)
		printf("error in creating a commit\n");
	printf("Commit created\n");
	

	git_repository_free(repo);
	return 0;
}
