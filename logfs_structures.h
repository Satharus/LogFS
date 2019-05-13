#ifndef LOGFS_LOGFS_STRUCTURES_H
#define LOGFS_LOGFS_STRUCTURES_H


#include <fuse.h>
#include <stddef.h>
#include <dirent.h>

#define LOGFILEPATH "/tmp/LogFS_Log.txt"

struct logfs_dirPointer {
	 DIR *directoryPointer;
	 struct dirent *directoryEntry;
	 off_t offset;
};


//Struct that holds the command line options
static struct options {
	 int disableLogging; //Disables logging
	 int showHelp;
	 int show_version;
	 int showLogs;
	 int removeLogs;
} fuseOptions;

//Struct that holds the mounting point for the filesystem
static struct mountpoint {
	 int fileDescriptor;
	 struct logfs_dirPointer *dir;
	 char *path;
} mountpoint;

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }
/*
 * This basically expands OPTION(t, p) to:
 * {
 * 	t, //The specified template to be set in the fuse_opt struct
 *
 * 	offsetof(struct options, p), //Gets the offset of the specified option from options
 *
 * 	1  //Ignored value, see fuse_opt.h for more info
 * }
 *
 */

//Array of fuse_opt structs for holding the command line options
//Each OPTION contains the specified option and the variable from struct options to set
static const struct fuse_opt option_spec[] = {
		  OPTION("--disable-logging", disableLogging),
		  OPTION("-h", showHelp),
		  OPTION("--help", showHelp),
		  OPTION("-V", show_version),
		  OPTION("--version", show_version),
		  OPTION("-l", showLogs),
		  OPTION("--show-logs", showLogs),
		  OPTION("-r", removeLogs),
		  OPTION("--reset-logs", removeLogs),
		  FUSE_OPT_END
};

//Returns the a directory pointer using the filehandle id for the specified file
static inline struct logfs_dirPointer *get_dirptr(struct fuse_file_info *fileInfo)
{
	return (struct logfs_dirPointer *) (uintptr_t) fileInfo->fh;
}

#endif //LOGFS_LOGFS_STRUCTURES_H