
#include <fuse_opt.h>
#include "fg.h"
#include "fg_repo.h"




struct fg_config_opts {
	const char *backup;
	const char *restore;
	const char *repo_path;
	int debug;
};

/**
 * USAGE
 */
	static void
usage(void)
{
	fprintf(stdout, "FUSEGIT usage:\n");
	fprintf(stdout, "fusegit [OPTION]... [MOUNTPOINT]\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "DESCRIPTION:\n");
	fprintf(stdout, "\tOptions can include backup or restore. Only one task can be performed at a time, either mount a file system, take backup or do restore.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\t-b, --backup\n");
	fprintf(stdout, "\t\tThis takes a compulsory argument, which is the mountpoint for the file system you want to take backup.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\t-d, --debug\n");
	fprintf(stdout, "\t\tThis should not have any argument\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\t-m, --mount\n");
	fprintf(stdout, "\t\tThis takes a compulsory argument, which is the mountpoint for the file system.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\t-r, --restore\n");
	fprintf(stdout, "\t\tThis takes a compulsory argument, which is the mountpoint for the file system you want to restore.\n");
}

/**
 * remove any trailing / from the mpoint which is returned
 */
	static void
get_mountpoint(const char *arg, char *mpoint)
{
	char cwd[PATH_MAX_LENGTH];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
		DEBUG("current working directory: %s", cwd);
	DEBUG("function is called for %s", arg);

	const char *y = cwd;
	while ((*mpoint = *y)) {
		mpoint++;
		y++;
	}
	y = arg;
	*mpoint = '/'; mpoint++;
	while ((*mpoint = *y)) {
		mpoint++;
		y++;
	}
}

/**
 * Process the mountpoint which the user has provided and then use it to create
 * a git repository in the same parent folder.
 * 
 * data is config_opts
 */
	static int
fusegit_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	struct fg_config_opts *opts = (struct fg_config_opts *)data;
	switch(key) {
	case FG_OPT_MOUNT:
		DEBUG("THIS SHOULD BE MOUNTPOINT: %s", arg);
		if (opts->repo_path)
			return -1;
		opts->repo_path = arg;
		DEBUG("MOUNTPOINT: success");
		return 0;
	case FG_OPT_BACKUP:
		DEBUG("BACKUP THIS MOUNTPOINT: %s", arg);
		if (opts->backup)
			return -1;
		opts->backup = arg;
		DEBUG("BACKUP: success");
		return 0;
	case FG_OPT_RESTORE:
		DEBUG("RESTORE THIS MOUNTPOINT: %s", arg);
		if (opts->restore)
			return -1;
		opts->restore = arg;
		DEBUG("RESTORE: success");
		return 0;
	case FG_OPT_DEBUG:
		DEBUG("DEBUG");
		if (opts->debug)
			return -1;
		opts->debug = 1;
		DEBUG("DEBUG: success");
		return 0;
	}
	// no other option is allowed
	return -1;
}

	int
main(int argc, char *argv[])
{
	int r;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fg_config_opts config_opts = { .backup = NULL, .restore = NULL,
		.repo_path = NULL, .debug = 0 };
	struct fuse_opt matching_opts[] = {
		FUSE_OPT_KEY("-b ", FG_OPT_BACKUP),
		FUSE_OPT_KEY("-r ", FG_OPT_RESTORE),
		FUSE_OPT_KEY("-m ", FG_OPT_MOUNT),
		FUSE_OPT_KEY("-d", FG_OPT_DEBUG),
		FUSE_OPT_KEY("--backup ", FG_OPT_BACKUP),
		FUSE_OPT_KEY("--restore ", FG_OPT_RESTORE),
		FUSE_OPT_KEY("--mount ", FG_OPT_MOUNT),
		FUSE_OPT_KEY("--debug", FG_OPT_DEBUG),
		FUSE_OPT_END
	};
	
	if ((r = fuse_opt_parse(&args, &config_opts, matching_opts,
		(fuse_opt_proc_t)fusegit_proc)) < 0) {
		usage();
		return r;
	}
	/*
		get_mountpoint(arg, repo_path);
		if (strlen(repo_path) + strlen(".repo") >= PATH_MAX_LENGTH-1)
			return -1;
		strcpy(repo_path+strlen(repo_path), ".repo");

		// Now we have obtained the address of the repository, we can set it
		if ((r = repo_setup(repo_path)) < 0)
			return -1;
		DEBUG("success");
	*/
	int i;
	for (i=0; i<args.argc; i++) {
		DEBUG("argument %d: %s", i, args.argv[i]);
	}
	fuse_opt_free_args(&args);

        return 0;
	//return fg_fuse_main(argc, argv);
}
