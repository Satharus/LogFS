#ifndef LOGFS_LOGFS_FUNCTIONS_H
#define LOGFS_LOGFS_FUNCTIONS_H

#include "logfs_structures.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <zconf.h>

static void *logfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg);

static int logfs_access(const char *path, int mask);

static int logfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

static int logfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset,
		  						struct fuse_file_info *fi, enum fuse_readdir_flags flags);

static int logfs_rmdir(const char *path);

static int logfs_unlink(const char *path);

DIR *openDirAt(int dirFileDescriptor,char const *path,int extraFlags);

static int logfs_opendir(const char *path,struct fuse_file_info *fi);

static int logfs_mkdir(const char *path, mode_t mode);

static int logfs_open(const char *path, struct fuse_file_info *fi);

static int logfs_read_buf(const char *path, struct fuse_bufvec **bufpointer,
								  size_t size, off_t offset, struct fuse_file_info *fi);

static int logfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

static int logfs_write_buf(const char *path, struct fuse_bufvec *buf,
		  							off_t offset, struct fuse_file_info *fi);

static void showHelp(const char *progname);

static void show_version(const char *progname);

void logfs_log_to_file(int operationID, const char *operand, char filePath[]);

#endif //LOGFS_LOGFS_FUNCTIONS_H