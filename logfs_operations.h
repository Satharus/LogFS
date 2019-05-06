#ifndef LOGFS_LOGFS_OPERATIONS_H
#define LOGFS_LOGFS_OPERATIONS_H

#include "logfs_functions.c"

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

#endif //LOGFS_LOGFS_OPERATIONS_H