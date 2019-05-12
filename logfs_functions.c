
#include "logfs_functions.h"
#include "shellfunctions.c"

static void *logfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	//Initialisation function
	(void) conn;
	cfg->use_ino = 1;
	cfg->nullpath_ok = 1;
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int logfs_access(const char *path, int mask)
{
	//This function checks the user's permission for a file relative to a file descriptor
	int retValue;

	char relativePath[strlen(path) +1]; //length +1 because '.' is added

	strcpy(relativePath, "."); //length +1 because '.' is added
	strcat(relativePath, path); //add '.' which is a link to the current directory

	//Check the permissions using the file descriptor and the relative path
	retValue = faccessat(mountpoint.fileDescriptor, relativePath, mask, AT_EACCESS);

	if (retValue == -1) return -errno;

	return 0;
}

static int logfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	//This function gets the file stats for a file.
	/* Stats such as:
	 * 	Permissions, type of file, owner, group owner, etc..
	 * 	basically the stuff you get when you run ls -l
	 * */
	int retValue = 0;
	(void) path;	// (void) to avoid compiler warnings.

	if (fi) retValue = fstat(fi->fh, stbuf); //Get file stat from fi if it exists
	else
	{
		char relativePath[strlen(path) + 1]; //length +1 because '.' is added

		strcpy(relativePath, ".");  //add '.' which is a link to the current directory
		strcat(relativePath, path); //Add the path to the relative path

		//Get file stat
		retValue = fstatat(mountpoint.fileDescriptor, relativePath, stbuf, AT_SYMLINK_NOFOLLOW);
	}

	//Return the number of the last error if this fn fails to retrieve stat
	if (retValue == -1) return -errno;

	return 0;
}

static int logfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		  						struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{

	//This function reads the content of the passed directory.

	(void) path;	//Reference is retrieved from fuse_file_info fi
	(void) flags;

	struct logfs_dirPointer *currentDirectoryPointer = get_dirptr(fi); //Get a directory pointer

	//If the passed offset is incorrect, seek to the correct offset and set it
	if (offset != currentDirectoryPointer->offset)
	{
		seekdir(currentDirectoryPointer->directoryPointer, offset);
		currentDirectoryPointer->directoryEntry = NULL;
		currentDirectoryPointer->offset = offset;
	}

	//Loop to get the file stats
	while (1)
	{
		struct stat st;
		off_t nextOffset;
		enum fuse_fill_dir_flags fill_flags = 0;

		if (!currentDirectoryPointer->directoryEntry)
		{
			currentDirectoryPointer->directoryEntry = readdir(currentDirectoryPointer->directoryPointer);
			if (!currentDirectoryPointer->directoryEntry) break;
		}

#ifdef HAVE_FSTATAT
		if (flags & FUSE_READDIR_PLUS)
		{
			int retValue;

			retValue = fstatat(dirfd(d->dp), d->entry->d_name, &st, AT_SYMLINK_NOFOLLOW);
			if (retValue != -1)
				fill_flags =  fill_flags | FUSE_FILL_DIR_PLUS;
		}
#endif
		if (!(fill_flags & FUSE_FILL_DIR_PLUS))
		{
			memset(&st, 0, sizeof(st));
			st.st_ino = currentDirectoryPointer->directoryEntry->d_ino;
			st.st_mode = currentDirectoryPointer->directoryEntry->d_type << 12;
		}

		nextOffset = telldir(currentDirectoryPointer->directoryPointer);

		if (filler(buf, currentDirectoryPointer->directoryEntry->d_name, &st, nextOffset, fill_flags))
			break;

		currentDirectoryPointer->directoryEntry = NULL;
		currentDirectoryPointer->offset = nextOffset;
	}
	return 0;
}

static int logfs_unlink(const char *path)
{
	//This function unlinks a file from the filesystem (i.e. deletes it)
	int retValue;

	char relative_path[ strlen(path) + 1];
	strcpy(relative_path, ".");
	strcat(relative_path, path);


	retValue = unlinkat(mountpoint.fileDescriptor, relative_path, 0);
	if (retValue == -1)
		return -errno;

	return 0;
}

static int logfs_rmdir(const char *path)
{
	//This function unlinks a directory from the filesystem (i.e. deletes it)
	int retValue;

	char relativePath[ strlen(path) + 1];
	strcpy(relativePath, ".");
	strcat(relativePath, path);

	/*
	Use the regular unlink function, but with AT_REMOVEDIR which is a macro that signals
	the function to remove a directory not a file.
	*/
	retValue = unlinkat(mountpoint.fileDescriptor, relativePath, AT_REMOVEDIR);

	if (retValue == -1)
		return -errno;

	return 0;
}

DIR *openDirAt(int dirFileDescriptor,char const *path,int extraFlags)
{
	//This function returns a directory pointer for the given directory file descriptor

	//Do an OR operation with the directory macros and flags parameter to get the value of flags
	int openFlags=(O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOCTTY | O_NONBLOCK | extraFlags);

	char relativePath[strlen(path)+1];
	strcpy(relativePath,".");
	strcat(relativePath,path);

	//Get the new file descriptor using the flags and relative path
	int newFileDescriptor = openat(dirFileDescriptor, relativePath, openFlags);

	if(newFileDescriptor < 0) return NULL; //Directory doesn't exist

	//Get a directory pointer for the new file descriptor
	DIR *dirPointer = fdopendir(newFileDescriptor);

	//If getting the directory pointer failed for any reason
	if(!dirPointer)
	{
		int fileDescriptorOpenDirErrno = errno;
		close(newFileDescriptor);
		errno = fileDescriptorOpenDirErrno;
	}

	return dirPointer;
}

static int logfs_opendir(const char *path,struct fuse_file_info *fi)
{
	//This function changes the working directory to the specified path.
	int returnValue;

	//If the path is the root directory
	if (strcmp(path, "/") == 0)
	{

		//If the filesystem isn't mounted correctly
		if (mountpoint.dir == NULL)
		{
			returnValue = -errno;
			return returnValue;
		}

		//Set the file handle id to the mounting point
		fi->fh = (unsigned long) mountpoint.dir;
		return 0;

	}

	struct logfs_dirPointer *logfsDirPointer = malloc(sizeof(struct logfs_dirPointer));

	if(logfsDirPointer == NULL) return -ENOMEM;	//No memory to allocate a directory pointer.

	//Set the directory pointer to the specified path relative to the mounting point
	logfsDirPointer->directoryPointer = openDirAt(mountpoint.fileDescriptor, path, 0);

	//If the directory couldn't be opened
	if(logfsDirPointer->directoryPointer == NULL)
	{
		returnValue = -errno;
		free(logfsDirPointer);
		return returnValue;
	}

	logfsDirPointer->offset = 0;
	logfsDirPointer->directoryEntry = NULL;

	fi->fh = (unsigned long) logfsDirPointer;

	return 0;
}

static int logfs_mkdir(const char *path, mode_t mode)
{
	//Makes a directory at the specified path.

	int retValue;
	char relative_path[ strlen(path) + 1];
	strcpy(relative_path, ".");
	strcat(relative_path, path);
	retValue = mkdirat(mountpoint.fileDescriptor, relative_path, mode);

	if (retValue == -1) return -errno;
	return 0;
}

static int logfs_open(const char *path, struct fuse_file_info *fi)
{
	// Opens the file, makes sure that the file is readable, sets the file
	// handle ID to the file descriptor
	int fileDescriptor;
	char relativePath[strlen(path)+1];
	strcpy(relativePath,".");
	strcat(relativePath,path);

	fileDescriptor = openat(mountpoint.fileDescriptor,relativePath,fi->flags);

	if(fileDescriptor==-1)
		return -errno;

	fi->fh = fileDescriptor;
	return 0;
}

static int logfs_read_buf(const char *path, struct fuse_bufvec **bufpointer,
								  size_t size, off_t offset, struct fuse_file_info *fi)
{
	//This function reads the content of a file to a buffer vector.

	(void) path;

	//Data buffer vector :
	// An array of data buffers, each containing a memory pointer or a file descriptor.
	struct fuse_bufvec *source;


	source = malloc(sizeof(struct fuse_bufvec));
	if (source == NULL) return -ENOMEM; //Not enough memory

	*source = FUSE_BUFVEC_INIT(size);

	source->buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	source->buf[0].fd = fi->fh;
	source->buf[0].pos = offset;

	*bufpointer = source;
	return 0;
}

static int logfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	//Creates a new file, this runs when the user uses touch.

	int fileDescriptor;
	char relativePath[strlen(path)+1];
	strcpy(relativePath,".");
	strcat(relativePath,path);

	fileDescriptor = openat(mountpoint.fileDescriptor, relativePath, fi->flags, mode);
	if(fileDescriptor==-1) return -errno;

	fi->fh=fileDescriptor;
	return 0;
}

static int logfs_write_buf(const char *path, struct fuse_bufvec *buf,
									off_t offset, struct fuse_file_info *fi)
{
	//This function writes from a buffer vector to a file, used with editing files using
	//nano or vim.
	(void) path;

	struct fuse_bufvec destination = FUSE_BUFVEC_INIT(fuse_buf_size(buf));


	destination.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	destination.buf[0].fd = fi->fh;
	destination.buf[0].pos = offset;

	return fuse_buf_copy(&destination, buf, FUSE_BUF_SPLICE_NONBLOCK);
}

static void showHelp(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
			 "  --disable-logging      Disables logging\n"
			 "          (Logging is enabled by default)\n"
			 "\n");
}

static void show_version(const char *progname)
{
	printf("%s Built with fuse v%d\n", progname, FUSE_MAJOR_VERSION);
}

void logfs_log_to_file(int operationID, char operand[], char filePath[])
{
	/* operations
	 * 1) Mount
	 * 2) List
	 * 3) Create(directory)
	 * 4) Create(file)
	 * 5) Unmount
	 * 6) Change working directory
	 * 7) Remove
	 */

	char *userName = getShellCommandOutput("whoami | tr -d  \"\\n\"");
	char *dateAndTime = getShellCommandOutput("date");
	char *operation = "";
	if		  (operationID == 1)		operation = "mounted the filesystem";
	else if (operationID == 2)		operation = "listed the files in directory";
	else if (operationID == 3)		operation = "created a new directory: ";
	else if (operationID == 4)		operation = "created a new file: ";
	else if (operationID == 5)		operation = "unmounted the filesystem";
	else if (operationID == 6)		operation = "changed the working directory";
	else if (operationID == 7)		operation = "removed file from the filesystem: ";

	FILE *logFile = fopen(filePath, "a");
	fprintf(logFile, "User %s %s%s \t\t %s", userName, operation, operand, dateAndTime);
	fclose(logFile);

	removeLogDuplicates(filePath);
}