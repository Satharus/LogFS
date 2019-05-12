#ifndef LOGFS_LOGFS_OPERATIONS_H
#define LOGFS_LOGFS_OPERATIONS_H

#include "logfs_functions.c"

static struct fuse_operations logfs_oper = {
		  .init     = logfs_init,
		  .access	= logfs_access,
		  .getattr	= logfs_getattr,
		  .readdir	= logfs_readdir,
		  .opendir  = logfs_opendir,
		  .open		= logfs_open,
		  .mkdir		= logfs_mkdir,
		  .write_buf= logfs_write_buf,
		  .create   = logfs_create,
		  .read_buf = logfs_read_buf,
		  .rmdir		= logfs_rmdir,
		  .unlink	= logfs_unlink,
};

#endif //LOGFS_LOGFS_OPERATIONS_H