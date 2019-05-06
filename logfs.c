/*
 * Compile with:
 *     gcc -Wall logfs.c `pkg-config fuse3 --cflags --libs` -o LogFS
 */

#define FUSE_USE_VERSION 35

#include "logfs_operations.h"


int main(int argc, char *argv[])
{
	int returnValue;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */


	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	umask(0);

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
		returnValue = fuse_main(args.argc, args.argv, &logfs_oper, NULL);
		fuse_opt_free_args(&args);
		return returnValue;
	}

	if (options.show_version)
	{
		char *programName;
		programName = &argv[0][2]; //Removing the "./" at the start of argv[0]

		show_version(programName);
		assert(fuse_opt_add_arg(&args, "--version") == 0);
		args.argv[0][0] = '\0';
		returnValue = fuse_main(args.argc, args.argv, &logfs_oper, NULL);
		fuse_opt_free_args(&args);
		return returnValue;
	}

	mountpoint.path = fuse_mnt_resolve_path(strdup(argv[0]), argv[argc - 1]);
	mountpoint.dir = malloc(sizeof(struct logfs_dirPointer));
	if (mountpoint.dir == NULL)
		return -ENOMEM;

	mountpoint.dir->directoryPointer = opendir(mountpoint.path);
	if (mountpoint.dir->directoryPointer == NULL) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -1;
	}

	if ((mountpoint.fileDescriptor = dirfd(mountpoint.dir->directoryPointer)) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -1;
	}
	mountpoint.dir->offset = 0;
	mountpoint.dir->directoryEntry = NULL;

	returnValue = fuse_main(args.argc, args.argv, &logfs_oper, NULL);
	fuse_opt_free_args(&args);

	closedir(mountpoint.dir->directoryPointer);
	free(mountpoint.path);
	return returnValue;
}