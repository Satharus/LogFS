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
	   additional help (by adding `--help` to the options again

	   if the program is run without any arguments then print
	   LogFS specific help text only.
	*/
	if (fuseOptions.showHelp || argc < 2) {
		showHelp(argv[0]); //Show LogFS specific help
		if (argc >= 2)    //If --help or -h is specified
		{
			assert(fuse_opt_add_arg(&fuseArguments, "--help") == 0); //Get FUSE help
			fuseArguments.argv[0][0] = '\0';

			//Give control to fuse_main
			returnValue = fuse_main(fuseArguments.argc, fuseArguments.argv, &logfs_oper, NULL);
			fuse_opt_free_args(&fuseArguments); //Free the argument list

		}

		return returnValue;
	}

	if (fuseOptions.show_version)
	{
		char *programName;
		programName = &argv[0][2]; //Removing the "./" at the start of argv[0]

		show_version(programName); //Show LogFS specific version

		assert(fuse_opt_add_arg(&fuseArguments, "--version") == 0); //Get FUSE version
		fuseArguments.argv[0][0] = '\0';

		//Give control to fuse_main
		returnValue = fuse_main(fuseArguments.argc, fuseArguments.argv, &logfs_oper, NULL);
		fuse_opt_free_args(&fuseArguments); //Free the argument list
		return returnValue;
	}

	//Set the mount point to the last argument
	mountpoint.path = argv[argc - 1];

	//Allocate the directory pointer
	mountpoint.dir = malloc(sizeof(struct logfs_dirPointer));
	if (mountpoint.dir == NULL)
		return -ENOMEM; //No memory

	//Get a stream for the mounting point
	mountpoint.dir->directoryPointer = opendir(mountpoint.path);
	if (mountpoint.dir->directoryPointer == NULL) { //If the stream didn't initialise
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -1;
	}

	//If the DIR stream's file descriptor returned an error
	if ((mountpoint.fileDescriptor = dirfd(mountpoint.dir->directoryPointer)) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -1;
	}

	mountpoint.dir->offset = 0;
	mountpoint.dir->directoryEntry = NULL;

	//Set the log file
	char *logFile;
	logFile = malloc(sizeof(char) * 1024);
	strcat(logFile, mountpoint.path);
	strcat(logFile, "/Log.txt");
	sessionInfo.logFilePath = logFile;

	//Log that the filesystem has been mounted
	if (!fuseOptions.disableLogging) logfs_log_to_file(1, "", sessionInfo.logFilePath);

	//Give control to fuse_main
	returnValue = fuse_main(fuseArguments.argc, fuseArguments.argv, &logfs_oper, NULL);
	fuse_opt_free_args(&fuseArguments); //free arguments

	//Log that the filesystem has been unmounted
	if (!fuseOptions.disableLogging) logfs_log_to_file(5, "", sessionInfo.logFilePath);

	closedir(mountpoint.dir->directoryPointer); //close the directory
	free(mountpoint.path); //Free the path variable
	free(logFile);
	free(sessionInfo.logFilePath);
	return returnValue;
}