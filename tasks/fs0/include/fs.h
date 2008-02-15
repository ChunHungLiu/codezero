/*
 * VFS definitions.
 *
 * Copyright (C) 2007, 2008 Bahadir Balban.
 */
#ifndef __FS_H__
#define __FS_H__

#include <l4/lib/list.h>
#include <l4/macros.h>
#include <l4lib/types.h>
#include <stat.h>


typedef void (*vnode_op_t)(void);
typedef void (*file_op_t)(void);

struct dentry;
struct file;
struct file_system_type;
struct superblock;
struct vnode;

struct dentry_ops {
	int (*compare)(struct dentry *d, char *n);
};

/* Operations that work on file content */
struct file_ops {

	/* Read a vnode's contents by page range */
	int (*read)(struct vnode *v, unsigned long pfn,
		    unsigned long npages, void *buf);

	/* Write a vnode's contents by page range */
	int (*write)(struct vnode *v, unsigned long pfn,
		    unsigned long npages, void *buf);
	file_op_t open;
	file_op_t close;
	file_op_t mmap;
	file_op_t lseek;
	file_op_t flush;
	file_op_t fsync;
};

/* Operations that work on vnode fields and associations between vnodes */
struct vnode_ops {
	vnode_op_t create;
	struct vnode *(*lookup)(struct vnode *root, char *path);
	int (*readdir)(struct vnode *v);
	vnode_op_t link;
	vnode_op_t unlink;
	int (*mkdir)(struct vnode *parent, char *name);
	vnode_op_t rmdir;
	vnode_op_t rename;
	vnode_op_t getattr;
	vnode_op_t setattr;
};

struct superblock_ops {
	int (*write_sb)(struct superblock *sb);

	/*
	 * Given a vnum, reads the disk-inode and copies its data
	 * into the vnode's generic fields
	 */
	int (*read_vnode)(struct superblock *sb, struct vnode *v);

	/* Writes vnode's generic fields into the disk-inode structure */
	int (*write_vnode)(struct superblock *sb, struct vnode *v);

	/* Allocates a disk-inode along with a vnode, and associates the two */
	struct vnode *(*alloc_vnode)(struct superblock *sb);

	/* Frees the vnode and the corresponding on-disk inode */
	int (*free_vnode)(struct superblock *sb, struct vnode *v);
};

#define	VFS_DNAME_MAX			256
struct dentry {
	int refcnt;
	char name[VFS_DNAME_MAX];
	struct dentry *parent;		/* Parent dentry */
	struct list_head child;		/* List of dentries with same parent */
	struct list_head children;	/* List of children dentries */
	struct list_head vref;		/* For vnode's dirent reference list */
	struct list_head cache_list;	/* Dentry cache reference */
	struct vnode *vnode;		/* The vnode associated with dirent */
	struct dentry_ops ops;
};

/*
 * Buffer to keep directory content. This is the only vnode content
 * that fs0 maintains. All other file data is in mm0 page cache.
 */
struct dirbuf {
	unsigned long npages;
	int dirty;
	u8 *buffer;
};

struct vnode {
	unsigned long vnum;		/* Filesystem-wide unique vnode id */
	int refcnt;			/* Reference counter */
	int hardlinks;			/* Number of hard links */
	struct superblock *sb;		/* Reference to superblock */
	struct vnode_ops ops;		/* Operations on this vnode */
	struct file_ops fops;		/* File-related operations on this vnode */
	struct list_head dentries;	/* Dirents that refer to this vnode */
	struct list_head cache_list;	/* For adding the vnode to vnode cache */
	struct dirbuf dirbuf;		/* Only directory buffers are kept */
	u32 mode;			/* Permissions and vnode type */
	u32 owner;			/* Owner */
	u64 atime;			/* Last access time */
	u64 mtime;			/* Last content modification */
	u64 ctime;			/* Last vnode modification */
	u64 size;			/* Size of contents */
	void *inode;			/* Ptr to fs-specific inode */
};

/* FS0 vfs specific macros */

/* Check if directory */
#define vfs_isdir(v)		S_ISDIR((v)->mode)

/* Set vnode type */
#define vfs_set_type(v, type)	{v->mode &= ~S_IFMT; v->mode |= S_IFMT & type; }

struct fstype_ops {
	struct superblock *(*get_superblock)(void *buf);
};

#define VFS_FSNAME_MAX			256
struct file_system_type {
	char name[VFS_FSNAME_MAX];
	unsigned long magic;
	struct fstype_ops ops;
	struct list_head list;	/* Member of list of all fs types */
	struct list_head sblist; /* List of superblocks with this type */
};
struct superblock *get_superblock(void *buf);

struct superblock {
	u64 fssize;
	unsigned int blocksize;
	struct list_head list;
	struct file_system_type *fs;
	struct superblock_ops *ops;
	struct vnode *root;
	void *fs_super;
};

void vfs_mount_fs(struct superblock *sb);

extern struct dentry_ops generic_dentry_operations;

#endif /* __FS_H__ */
