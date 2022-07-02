#include "lib.h"
#include <fs.h>

#define debug 0

static int file_close(struct Fd *fd);
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);
static int file_stat(struct Fd *fd, struct Stat *stat);


// Dot represents choosing the variable of the same name within struct declaration
// to assign, and no need to consider order of variables.
struct Dev devfile = {
	.dev_id =	'f',
	.dev_name =	"file",
	.dev_read =	file_read,
	.dev_write =	file_write,
	.dev_close =	file_close,
	.dev_stat =	file_stat,
};


// Overview:
//	Open a file (or directory).
//
// Returns:
//	the file descriptor onsuccess,
//	< 0 on failure.
/*** exercise 5.8 ***/
int
open(const char *path, int mode)
{
	struct Fd *fd;
	struct Filefd *ffd;
	u_int size, fileid;
	int r;
	u_int va;
	u_int i;

	// Step 1: Alloc a new Fd, return error code when fail to alloc.
	// Hint: Please use fd_alloc.
	r = fd_alloc(&fd);
	if (r < 0) return r;

	// Step 2: Get the file descriptor of the file to open.
	// Hint: Read fsipc.c, and choose a function.
	r = fsipc_open(path, mode, fd);

	if(debug) writef("finish open\n");
	if (r < 0) return r;
	ffd = (struct FileFd *)fd;

	// Step 3: Set the start address storing the file's content. Set size and fileid correctly.
	// Hint: Use fd2data to get the start address.
	va = fd2data(fd);

	// Step 4: Alloc memory, map the file content into memory.
	size = (ffd->fstype == 0) ? ffd->f_file.f_size : ffd->f_FATfile.DIR_FileSize;

	if(debug) writef("size = %d, DIREnt_size = %d, fstype = %d\n", size, ffd->f_FATfile.DIR_FileSize, ffd->fstype);
	// writef("EXT_Size = %d\n", ffd->f_file.f_size);

	for (i = 0; i < size; i += BY2PG) {
		// writef("Map page %d.\n", i);
		fsipc_map(ffd->f_fileid, i, va + i);
		// each time map a single page for va.
	}
	if(debug) writef("Finish Map!\n");

	// Step 5: Return the number of file descriptor.
	return fd2num(fd);
}

// Overview:
//	Close a file descriptor
int
file_close(struct Fd *fd)
{
	int r;
	struct Filefd *ffd;
	u_int va, size, fileid;
	u_int i;

	ffd = (struct Filefd *)fd;
	fileid = ffd->f_fileid;
	size = (ffd->fstype == 0) ? ffd->f_file.f_size : ffd->f_FATfile.DIR_FileSize;

	// Set the start address storing the file's content.
	va = fd2data(fd);

	// Tell the file server the dirty page.
	for (i = 0; i < size; i += BY2PG) {
		fsipc_dirty(fileid, i);
	}

	// Request the file server to close the file with fsipc.
	if ((r = fsipc_close(fileid)) < 0) {
		// writef("cannot close the file\n");
		return r;
	}

	// Unmap the content of file, release memory.
	if (size == 0) {
		return 0;
	}
	for (i = 0; i < size; i += BY2PG) {
		if ((r = syscall_mem_unmap(0, va + i)) < 0) {
			// writef("cannont unmap the file.\n");
			return r;
		}
	}
	return 0;
}

// Overview:
//	Read 'n' bytes from 'fd' at the current seek position into 'buf'. Since files
//	are memory-mapped, this amounts to a user_bcopy() surrounded by a little red
//	tape to handle the file size and seek pointer.
static int
file_read(struct Fd *fd, void *buf, u_int n, u_int offset)
{
	u_int size;
	struct Filefd *ffd;
	ffd = (struct Filefd *)fd;

	// Avoid reading past the end of file.
	size = (ffd->fstype == 0) ? ffd->f_file.f_size : ffd->f_FATfile.DIR_FileSize;

	if (offset > size) {
		return 0;
	}

	// 如果n大于剩下要读的内容，就修改n为之后内容的长度
	if (offset + n > size) {
		n = size - offset;
	}

	user_bcopy((char *)fd2data(fd) + offset, buf, n);
	return n;
	// 返回读或写的字节数
}

// Overview:
//	Find the virtual address of the page that maps the file block
//	starting at 'offset'.
int
read_map(int fdnum, u_int offset, void **blk)
{
	int r;
	u_int va;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	va = fd2data(fd) + offset;

	if (offset >= MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if (!((* vpd)[PDX(va)]&PTE_V) || !((* vpt)[VPN(va)]&PTE_V)) {
		return -E_NO_DISK;
	}

	*blk = (void *)va;
	return 0;
}

// Overview:
//	Write 'n' bytes from 'buf' to 'fd' at the current seek position.
static int
file_write(struct Fd *fd, const void *buf, u_int n, u_int offset)
{
	int r, size;
	u_int tot;
	struct Filefd *ffd;

	ffd = (struct Filefd *)fd;
	size = (ffd->fstype == 0) ? ffd->f_file.f_size : ffd->f_FATfile.DIR_FileSize;

	// Don't write more than the maximum file size.
	tot = offset + n;

	if (tot > MAXFILESIZE) {
		return -E_NO_DISK;
	}

	// Increase the file's size if necessary
	if (tot > size) {
		if ((r = ftruncate(fd2num(fd), tot)) < 0) {
			return r;
		}
	}
	// writef("after write");
	// Write the data
	user_bcopy(buf, (char *)fd2data(fd) + offset, n);
	return n;
}

static int
file_stat(struct Fd *fd, struct Stat *st)
{
	struct Filefd *f;

	f = (struct Filefd *)fd;

	strcpy(st->st_name, (char *)((f->fstype==0) ? f->f_file.f_name : f->f_FATfile.DIR_Name));
	st->st_size = (f->fstype == 0) ? f->f_file.f_size : f->f_FATfile.DIR_FileSize;
	
	if (f->fstype == 0) {
		st->st_isdir = (f->f_file.f_type == FTYPE_DIR);
	}
	else {
		st->st_isdir = ((f->f_FATfile.DIR_Attr & ATTR_DIRECTORY) != 0);
	}
	return 0;
}

// Overview:
//	Truncate or extend an open file to 'size' bytes
// 截断或扩展一个文件的大小
int
ftruncate(int fdnum, u_int size)
{
	int i, r;
	struct Fd *fd;
	struct Filefd *f;
	u_int oldsize, va, fileid;

	if (size > MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	f = (struct Filefd *)fd;
	fileid = f->f_fileid;

	if (f->fstype == 0) {
		oldsize = f->f_file.f_size;
		f->f_file.f_size = size;
	} else {
		oldsize = f->f_FATfile.DIR_FileSize;
		f->f_FATfile.DIR_FileSize = size;
	}

	/* Step1: Ask for more size. Mapped in fs_serv process. */
	if ((r = fsipc_set_size(fileid, size)) < 0) {
		return r;
	}
	// writef("set_size");
	va = fd2data(fd);

	/* Step2: Map the extended content into my space.(Only extend can happen.) */
	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, BY2PG); i < ROUND(size, BY2PG); i += BY2PG) {
		if ((r = fsipc_map(fileid, i, va + i)) < 0) {
			fsipc_set_size(fileid, oldsize);
			return r;
		}
	}

	/* Step3: Unmap pages if truncating the file. */
	for (i = ROUND(size, BY2PG); i < ROUND(oldsize, BY2PG); i += BY2PG)
		if ((r = syscall_mem_unmap(0, va + i)) < 0) {
			user_panic("ftruncate: syscall_mem_unmap %08x: %e", va + i, r);
		}

	return 0;
}

// Overview:
//	Delete a file or directory.
/*** exercise 5.10 ***/
int
remove(const char *path)
{
	// Your code here.
	// Call fsipc_remove.
	return fsipc_remove(path);
}

// Overview:
//	Synchronize disk with buffer cache
int
sync(void)
{
	return fsipc_sync();
}
