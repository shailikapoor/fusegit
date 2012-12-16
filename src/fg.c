
#include <fuse_opt.h>
#include "fg.h"
#include "fg_repo.h"




struct fg_config_opts {
	enum fg_task task;
	char *backup;
	char *restore;
	char *mount;
	char *name;
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
	fprintf(stdout, "\t-n, --name\n");
	fprintf(stdout, "\t\tThis is a compulsory option if either backup or resotre is done. This is the name of the snapshot.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "\t-r, --restore\n");
	fprintf(stdout, "\t\tThis takes a compulsory argument, which is the mountpoint for the file system you want to restore.\n");
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
	int off;
	switch(key) {
	case FG_OPT_MOUNT:
		DEBUG("THIS SHOULD BE MOUNTPOINT: %s", arg);
		if (opts->mount)
			return -1;
		off = 2;
		if (arg[1] == '-')
			off = strlen("--mount");
		opts->mount = malloc(strlen(arg));
		strcpy(opts->mount, arg+off);
		opts->task = FG_MOUNT;
		DEBUG("MOUNTPOINT: success: %s", opts->mount);
		return 0;
	case FG_OPT_BACKUP:
		DEBUG("BACKUP THIS MOUNTPOINT: %s", arg);
		if (opts->backup)
			return -1;
		off = 2;
		if (arg[1] == '-')
			off = strlen("--backup");
		opts->backup = malloc(strlen(arg));
		strcpy(opts->backup, arg+off);
		opts->task = FG_BACKUP;
		DEBUG("BACKUP: success: %s", opts->backup);
		return 0;
	case FG_OPT_RESTORE:
		DEBUG("RESTORE THIS MOUNTPOINT: %s", arg);
		if (opts->restore)
			return -1;
		off = 2;
		if (arg[1] == '-')
			off = strlen("--restore");
		opts->restore = malloc(strlen(arg));
		strcpy(opts->restore, arg+off);
		opts->task = FG_RESTORE;
		DEBUG("RESTORE: success: %s", opts->restore);
		return 0;
	case FG_OPT_NAME:
		DEBUG("NAME OF THE SNAPSHOT: %s", arg);
		if (opts->name)
			return -1;
		off = 2;
		if (arg[1] == '-')
			off = strlen("--name");
		opts->name = malloc(strlen(arg));
		strcpy(opts->name, arg+off);
		DEBUG("NAME: success: %s", opts->name);
		return 0;
	case FG_OPT_DEBUG:
		DEBUG("DEBUG");
		if (opts->debug)
			return -1;
		opts->debug = 1;
		DEBUG("DEBUG: success");
		return 0;
	case FG_OPT_INVALID:
		DEBUG("INVALID: %s", arg);
		return -1;
	}
	// no other option is allowed
	return -1;
}

	int
validate_args(struct fg_config_opts *config_opts)
{
	int backup = config_opts->backup ? 1 : 0;
	int restore = config_opts->restore ? 1 : 0;
	int mount = config_opts->mount ? 1 : 0;
	int name = config_opts->name ? 1 : 0;

	int sum = backup + restore + mount;

	DEBUG("backup... %d", backup);
	DEBUG("restore... %d", restore);
	DEBUG("mount... %d", mount);
	DEBUG("name... %d", name);
	if (sum != 1)
		return -1;
	DEBUG("checking if backup...");
	if ((backup && name) || !(backup || name))
		return 0;
	DEBUG("checking if restore...");
	if ((restore && name) || !(restore || name))
		return 0;
	return 0;
}

/**
 * remove any trailing / from the mount which is returned
 */
	static void
get_mountpoint(char *mount)
{
	int len = strlen(mount);
	while (mount[len-1] == '/') {
		mount[len-1] = '\0';
		len = strlen(mount);
	}
}

	static int
setup(char *mount)
{
	int r;
	char mount_repo[PATH_MAX_LENGTH];
	get_mountpoint(mount);
	if (strlen(mount) + strlen(".repo") >= PATH_MAX_LENGTH-1)
		return -1;
	strcpy(mount_repo, mount);
	strcpy(mount_repo+strlen(mount), ".repo");

	// Now we have obtained the address of the repository, we can set it
	if ((r = repo_setup(mount_repo)) < 0)
		return -1;
	DEBUG("REPOSITORY SETUP IS SUCCESS");

	return 0;
}


/**
 * TODO Have the repository creation code in this file instead of fg_fuse.c for
 * both mount and backup.
 * TODO The repo file is right now hardcoded, but should be read from a config
 * file, so that it can be customized.
 * 
 * TODO how to handle umask
 */
	int
main(int argc, char *argv[])
{
	int r;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fg_config_opts config_opts = { .task = FG_MOUNT, .backup = NULL, .restore = NULL,
		.mount = NULL, .name = NULL, .debug = 0 };
	struct fuse_opt matching_opts[] = {
		FUSE_OPT_KEY("-b ", FG_OPT_BACKUP),
		FUSE_OPT_KEY("-r ", FG_OPT_RESTORE),
		FUSE_OPT_KEY("-m ", FG_OPT_MOUNT),
		FUSE_OPT_KEY("-n ", FG_OPT_NAME),
		FUSE_OPT_KEY("-d", FG_OPT_DEBUG),
		FUSE_OPT_KEY("--backup ", FG_OPT_BACKUP),
		FUSE_OPT_KEY("--restore ", FG_OPT_RESTORE),
		FUSE_OPT_KEY("--mount ", FG_OPT_MOUNT),
		FUSE_OPT_KEY("--name ", FG_OPT_NAME),
		FUSE_OPT_KEY("--debug", FG_OPT_DEBUG),
		FUSE_OPT_KEY("-mount ", FG_OPT_INVALID),
		FUSE_OPT_KEY("-backup ", FG_OPT_INVALID),
		FUSE_OPT_KEY("-restore ", FG_OPT_INVALID),
		FUSE_OPT_KEY("-name ", FG_OPT_INVALID),
		FUSE_OPT_KEY("--mount=", FG_OPT_INVALID),
		FUSE_OPT_KEY("--backup=", FG_OPT_INVALID),
		FUSE_OPT_KEY("--restore=", FG_OPT_INVALID),
		FUSE_OPT_KEY("--name=", FG_OPT_INVALID),
		FUSE_OPT_KEY("-m=", FG_OPT_INVALID),
		FUSE_OPT_KEY("-b=", FG_OPT_INVALID),
		FUSE_OPT_KEY("-r=", FG_OPT_INVALID),
		FUSE_OPT_KEY("-n=", FG_OPT_INVALID),
		FUSE_OPT_END
	};
	
	if ((r = fuse_opt_parse(&args, &config_opts, matching_opts,
		(fuse_opt_proc_t)fusegit_proc)) < 0) {
		usage();
		return r;
	}
	DEBUG("VALIDATING ARGS...");
	if ((r = validate_args(&config_opts)) < 0) {
		usage();
		return r;
	}

	int i;
	for (i=0; i<args.argc; i++) {
		DEBUG("argument %d: %s", i, args.argv[i]);
	}
	fuse_opt_free_args(&args);

	//DEBUG("*** IMPLEMENT THE repo_rename_file and repo_rename_dir functions");
	///*
	switch(config_opts.task) {
	case FG_MOUNT:
		DEBUG("mounting...%s", config_opts.mount);
		argc = 2;
		argv[argc-1] = config_opts.mount;
		if (config_opts.debug) {
			argc++;
			argv[argc-1] = "-d";
		}
		argc++;
		argv[argc-1] = "-s";
		if ((r = setup(config_opts.mount)) < 0)
			return -1;
		if ((r = fg_fuse_main(argc, argv)) < 0)
			return -1;
		break;
	case FG_BACKUP:
		DEBUG("backing up...%s snapshot:%s", config_opts.backup,
			config_opts.name);
		if ((r = setup(config_opts.backup)) < 0)
			return -1;
		if ((r = repo_backup(config_opts.name)) < 0)
			return -1;
		DEBUG("BACKUP SUCCESSFUL! mount:%s snapshot:%s",
			config_opts.backup, config_opts.name);
		break;
	case FG_RESTORE:
		DEBUG("restoring...%s snapshot:%s", config_opts.restore,
			config_opts.name);
		if ((r = setup(config_opts.restore)) < 0)
			return -1;
		if ((r = repo_restore(config_opts.name)) < 0)
			return -1;
		DEBUG("RESTORE SUCCESSFUL! mount:%s snapshot:%s",
			config_opts.restore, config_opts.name);
		break;
	}
	//*/

	if (config_opts.mount) free(config_opts.mount);
	if (config_opts.backup) free(config_opts.backup);
	if (config_opts.restore) free(config_opts.restore);
	if (config_opts.name) free(config_opts.name);
        return 0;
}
