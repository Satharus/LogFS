/*
 * Compile with:
 *     gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 */


#define FUSE_USE_VERSION 35

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <dirent.h>
#include <zconf.h>

struct logfs_dirPointer {
	 DIR *directoryPointer;
	 struct dirent *directoryEntry;
	 off_t offset;
};


/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
	 int enable_logging;
	 int show_help;
} options;

static struct mountpoint {
	 int fileDescriptor;
	 struct logfs_dirPointer *dir;
	 char *path;
} mountpoint;

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }

static const struct fuse_opt option_spec[] = {
		  OPTION("--enable-logging=%d", enable_logging),
		  OPTION("-h", show_help),
		  OPTION("--help", show_help),
		  FUSE_OPT_END
};

static inline struct logfs_dirPointer *get_dirptr(struct fuse_file_info *fileInfo)
{
	return (struct logfs_dirPointer *) (uintptr_t) fileInfo->fh;
}


static void *logfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;
	cfg->nullpath_ok = 1;
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int logfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	int res = 0;
	(void) path;

	if (fi) res = fstat(fi->fh, stbuf);
	else
	{
		char relativePath[strlen(path) + 1];

		strcpy(relativePath, ".");
		strcat(relativePath, path);

		res = fstatat(mountpoint.fileDescriptor, relativePath, stbuf, AT_SYMLINK_NOFOLLOW);
	}

	if (res == -1) return -errno;

	return 0;
}

static int logfs_access(const char *path, int mask)
{
	int res;

	char relativePath[strlen(path) +1];

	strcpy(relativePath, ".");
	strcat(relativePath, path);

	res = faccessat(mountpoint.fileDescriptor, relativePath, mask, AT_EACCESS);

	if (res == -1) return -errno;

	return 0;
}

static int logfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
								 off_t offset, struct fuse_file_info *fi,
								 enum fuse_readdir_flags flags)
{
	//LS - Lists the directory's content
	(void) path;
	(void) flags;
	struct logfs_dirPointer *dpr = get_dirptr(fi);
	if (offset != dpr->offset)
	{
#ifndef __FreeBSD__
		seekdir(dpr->directoryPointer, offset);
#else
		seekdir(dpr->directoryPointer, offset - 1);
#endif
		dpr->directoryEntry = NULL;
		dpr->offset = offset;
	}

	while (1)
	{
		struct stat st;
		off_t nextOffset;
		enum fuse_fill_dir_flags fill_flags = 0;

		if (!dpr->directoryEntry)
		{
			dpr->directoryEntry = readdir(dpr->directoryPointer);
			if (!dpr->directoryEntry) break;
		}
#ifdef HAVE_FSTATAT
		if (flags & FUSE_READDIR_PLUS) {
			int res;

			res = fstatat(dirfd(d->dp), d->entry->d_name, &st,
				      AT_SYMLINK_NOFOLLOW);
			if (res != -1)
				fill_flags |= FUSE_FILL_DIR_PLUS;
		}
#endif
		if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
			memset(&st, 0, sizeof(st));
			st.st_ino = dpr->directoryEntry->d_ino;
			st.st_mode = dpr->directoryEntry->d_type << 12;
		}
		nextOffset = telldir(dpr->directoryPointer);
#ifdef __FreeBSD__
		nextOffset++;
#endif
		if (filler(buf, dpr->directoryEntry->d_name, &st, nextOffset, fill_flags))
			break;

		dpr->directoryEntry = NULL;
		dpr->offset = nextOffset;
	}
	return 0;
}

static int logfs_rmdir(const char *path)
{
	int res;

	char relative_path[ strlen(path) + 1];
	strcpy(relative_path, ".");
	strcat(relative_path, path);

	res = unlinkat(mountpoint.fileDescriptor, relative_path, AT_REMOVEDIR);

	/* res = rmdir(path); */
	if (res == -1)
		return -errno;

	return 0;
}

static int logfs_unlink(const char *path)
{
	int res;

	char relative_path[ strlen(path) + 1];
	strcpy(relative_path, ".");
	strcat(relative_path, path);

	res = unlinkat(mountpoint.fileDescriptor, relative_path, 0);
	if (res == -1)
		return -errno;

	return 0;
}

static int logfs_releasedir(const char *path, struct fuse_file_info *fi)
{
	struct logfs_dirPointer *d = get_dirptr(fi);
	(void) path;
	if (d->directoryPointer == mountpoint.dir->directoryPointer) {
		return 0;
	}

	closedir(d->directoryPointer);
	free(d);
	return 0;
}
DIR *openDirAt(int dirFileDescriptor,char const *path,int extraFlags)
{
	int openFlags=(O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOCTTY
						| O_NONBLOCK | extraFlags);

	char relativePath[strlen(path)+1];
	strcpy(relativePath,".");
	strcat(relativePath,path);

	int newFileDescriptor=openat(dirFileDescriptor,relativePath,openFlags);

	if(newFileDescriptor<0)
		return NULL;
	DIR *dirPointer=fdopendir(newFileDescriptor);
	if(!dirPointer)
	{
		int fileDescriptorOpenDirErrno=errno;
		close(newFileDescriptor);
		errno=fileDescriptorOpenDirErrno;
	}
	return dirPointer;
}

static int logfs_opendir(const char *path,struct fuse_file_info *fi)
{
	int returnValue;
	if (strcmp(path, "/") == 0)
	{

		if (mountpoint.dir == NULL)
		{
			returnValue = -errno;
			return returnValue;
		}

		fi->fh = (unsigned long) mountpoint.dir;
		return 0;

	}
	struct logfs_dirPointer *d= malloc(sizeof(struct logfs_dirPointer));
	if(d==NULL)
		return -ENOMEM;
	d->directoryPointer=openDirAt(mountpoint.fileDescriptor, path, 0);
	if(d->directoryPointer == NULL)
	{
		returnValue=-errno;
		free(d);
		return returnValue;
	}
	d->offset = 0;
	d->directoryEntry = NULL;
	fi->fh = (unsigned long) d;
	return 0;

}

static int logfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int returnValue;

	if (S_ISFIFO(mode))
		returnValue = mkfifo(path, mode);
	else
		returnValue = mknod(path, mode, rdev);
	if (returnValue == -1)
		return -errno;

	return 0;
}

static int logfs_mkdir(const char *path, mode_t mode)
{
	int returnValue;
	char relative_path[ strlen(path) + 1];
	strcpy(relative_path, ".");
	strcat(relative_path, path);

	returnValue = mkdirat(mountpoint.fileDescriptor, relative_path, mode);
	if (returnValue == -1)
		return -errno;

	return 0;
}


static int logfs_open(const char *path, struct fuse_file_info *fi)
{
	// Opens the file, makes sure that the file is readable.
	int fileDirectory;
	char relativePath[strlen(path)+1];
	strcpy(relativePath,".");
	strcat(relativePath,path);

	fileDirectory=openat(mountpoint.fileDescriptor,relativePath,fi->flags);
	if(fileDirectory==-1)
		return -errno;

	fi->fh=fileDirectory;
	return 0;

}

static int logfs_read(const char *path, char *buf, size_t size, off_t offset,
							 struct fuse_file_info *fi)
{
	//CAT - Reads the content of a file
	int returnValue;
	(void) path;
	returnValue=pread(fi->fh,buf,size,offset);

	if(returnValue==-1)
		returnValue=-errno;

	return returnValue;
}

static int logfs_read_buf(const char *path, struct fuse_bufvec **bufpointer,
									 size_t size, off_t offset, struct fuse_file_info *fi)
{
	//Data buffer vector :
	// An array of data buffers, each containing a memory pointer or a file descriptor.
	struct fuse_bufvec *source;

	(void) path;

	source = malloc(sizeof(struct fuse_bufvec));
	if (source == NULL)
		return -ENOMEM;

	*source = FUSE_BUFVEC_INIT(size);

	source->buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	source->buf[0].fd = fi->fh;
	source->buf[0].pos = offset;

	*bufpointer = source;

	return 0;
}


static int logfs_truncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	int res;

	if(fi)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(path, size);

	if (res == -1)
		return -errno;

	return 0;
}

static int logfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fileDirectory;
	char relativePath[strlen(path)+1];
	strcpy(relativePath,".");
	strcat(relativePath,path);

	fileDirectory=openat(mountpoint.fileDescriptor,relativePath,fi->flags,mode);
	if(fileDirectory==-1)
		return -errno;

	fi->fh=fileDirectory;
	return 0;
}

static int logfs_write(const char *path, const char *buf, size_t size,
								 off_t offset, struct fuse_file_info *fi)
{
	int returnValue;

	(void) path;
	returnValue = pwrite(fi->fh, buf, size, offset);
	if (returnValue == -1)
		returnValue = -errno;

	return returnValue;
}
static int logfs_write_buf(const char *path, struct fuse_bufvec *buf,
									  off_t offset, struct fuse_file_info *fi)
{
	struct fuse_bufvec destantion = FUSE_BUFVEC_INIT(fuse_buf_size(buf));

	(void) path;

	destantion.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	destantion.buf[0].fd = fi->fh;
	destantion.buf[0].pos = offset;

	return fuse_buf_copy(&destantion, buf, FUSE_BUF_SPLICE_NONBLOCK);
}

static int logfs_statfs(const char *path, struct statvfs *stbuf)
{
	int returnValue;

	returnValue = fstatvfs(mountpoint.fileDescriptor, stbuf);

	if (returnValue == -1)
		return -errno;

	return 0;
}

static int logfs_flush(const char *path, struct fuse_file_info *fi)
{
	int returnValue;

	(void) path;
	/* This is called from every close on an open file, so call the
	   close on the underlying filesystem.	But since flush may be
	   called multiple times for an open file, this must not really
	   close the file.  This is important if used on a network
	   filesystem like NFS which flush the data/metadata on close() */
	returnValue = close(dup(fi->fh));
	if (returnValue == -1)
		return -errno;

	return 0;
}


static struct fuse_operations logfs_oper = {
		  .init     = logfs_init,
		  .truncate = logfs_truncate,
		  .access	= logfs_access,
		  .getattr	= logfs_getattr,
		  .readdir	= logfs_readdir,
		  .opendir  = logfs_opendir,
		  .open		= logfs_open,
		  .read		= logfs_read,
		  .mkdir		= logfs_mkdir,
		  .mknod		= logfs_mknod,
		  .write_buf= logfs_write_buf,
		  .write		= logfs_write,
		  .create   = logfs_create,
		  .read_buf = logfs_read_buf,
		  .statfs   = logfs_statfs,
		  .flush		= logfs_flush,
		  .rmdir		= logfs_rmdir,
		  .unlink	= logfs_unlink,
		  .releasedir=logfs_releasedir,
};

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
			 "\n");
}

char *fuse_mnt_resolve_path(const char *progname, const char *orig)
{
	char buf[PATH_MAX];
	char *copy;
	char *dst;
	char *end;
	char *lastcomp;
	const char *toresolv;

	if (!orig[0]) {
		fprintf(stderr, "%s: invalid mountpoint '%s'\n", progname,
				  orig);
		return NULL;
	}

	copy = strdup(orig);
	if (copy == NULL) {
		fprintf(stderr, "%s: failed to allocate memory\n", progname);
		return NULL;
	}

	toresolv = copy;
	lastcomp = NULL;
	for (end = copy + strlen(copy) - 1; end > copy && *end == '/'; end --);
	if (end[0] != '/') {
		char *tmp;
		end[1] = '\0';
		tmp = strrchr(copy, '/');
		if (tmp == NULL) {
			lastcomp = copy;
			toresolv = ".";
		} else {
			lastcomp = tmp + 1;
			if (tmp == copy)
				toresolv = "/";
		}
		if (strcmp(lastcomp, ".") == 0 || strcmp(lastcomp, "..") == 0) {
			lastcomp = NULL;
			toresolv = copy;
		}
		else if (tmp)
			tmp[0] = '\0';
	}
	if (realpath(toresolv, buf) == NULL) {
		fprintf(stderr, "%s: bad mount point %s: %s\n", progname, orig,
				  strerror(errno));
		free(copy);
		return NULL;
	}
	if (lastcomp == NULL)
		dst = strdup(buf);
	else {
		dst = (char *) malloc(strlen(buf) + 1 + strlen(lastcomp) + 1);
		if (dst) {
			unsigned buflen = strlen(buf);
			if (buflen && buf[buflen-1] == '/')
				sprintf(dst, "%s%s", buf, lastcomp);
			else
				sprintf(dst, "%s/%s", buf, lastcomp);
		}
	}
	free(copy);
	if (dst == NULL)
		fprintf(stderr, "%s: failed to allocate memory\n", progname);
	return dst;
}

int main(int argc, char *argv[])
{
	int ret;
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

	ret = fuse_main(args.argc, args.argv, &logfs_oper, NULL);
	fuse_opt_free_args(&args);

	closedir(mountpoint.dir->directoryPointer);
	free(mountpoint.path);
	return ret;
}
