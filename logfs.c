/*
 * Compile with:
 *     gcc -Wall logfs.c `pkg-config fuse3 --cflags --libs` -o LogFS
 */

#define FUSE_USE_VERSION 35

#include "logfs_operations.h"


int main(int argc, char *argv[])
{

	int returnValue;

	//Make  a fuse_args instance using the passed arguments
	struct fuse_args fuseArguments = FUSE_ARGS_INIT(argc, argv);

	//Parse command line options
	if (fuse_opt_parse(&fuseArguments, &fuseOptions, option_spec, NULL) == -1)
		return 1;

	//Sets the file mode creation mask, seen "man 2 umask" for more info
	umask(0);


	/*
		When --help is specified, first print LogFS'
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	*/
	if (fuseOptions.showHelp || argc < 2) {
		showHelp(argv[0]);
		if (argc >= 2)
		{
			assert(fuse_opt_add_arg(&fuseArguments, "--help") == 0);
			fuseArguments.argv[0][0] = '\0';
			returnValue = fuse_main(fuseArguments.argc, fuseArguments.argv, &logfs_oper, NULL);
			fuse_opt_free_args(&fuseArguments);
		}
		return returnValue;
	}

	if (fuseOptions.show_version)
	{
		char *programName;
		programName = &argv[0][2]; //Removing the "./" at the start of argv[0]

		show_version(programName);
		assert(fuse_opt_add_arg(&fuseArguments, "--version") == 0);
		fuseArguments.argv[0][0] = '\0';
		returnValue = fuse_main(fuseArguments.argc, fuseArguments.argv, &logfs_oper, NULL);
		fuse_opt_free_args(&fuseArguments);
		return returnValue;
	}

	//Print the status of the logging to the user.
	printf("Logging is %s.", fuseOptions.disableLogging ? "disabled" : "enabled");

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

	returnValue = fuse_main(fuseArguments.argc, fuseArguments.argv, &logfs_oper, NULL);
	fuse_opt_free_args(&fuseArguments);

	closedir(mountpoint.dir->directoryPointer);
	free(mountpoint.path);
	return returnValue;
}