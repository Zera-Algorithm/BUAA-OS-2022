/*
 * File system server main loop -
 * serves IPC requests from other environments.
 * 是一个类似服务器的进程，专门负责接收用户态的 **文件** 请求，然后调用fs.c中的函数实现。
 * 管理 文件打开请求、文件描述符等等。
 */

#include "fs.h"
#include "fd.h"
#include "lib.h"
#include <mmu.h>
#include <FAT.h>

struct Open {
	struct File *o_file;	// mapped descriptor for open file
	struct DIREnt *o_FATfile;
	int fstype; // 0: EXT; 1: FAT
	u_int o_fileid;		// file id
	int o_mode;		// open mode
	struct Filefd *o_ff;	// va of filefd page
};
// fileid是归属于系统层面的文件打开号，而fdnum是用户层面拥有的文件描述符。

// Max number of open files in the file system at once
#define MAXOPEN			1024
#define FILEVA 			0x60000000

// initialize to force into data section
struct Open opentab[MAXOPEN] = { { 0, 0, 1 } };

// Virtual address at which to receive page mappings containing client requests.
#define REQVA	0x0ffff000

// Overview:
//	Initialize file system server process.
void
serve_init(void)
{
	int i;
	u_int va;

	// Set virtual address to map.
	va = FILEVA;

	// Initial array opentab.
	for (i = 0; i < MAXOPEN; i++) {
		opentab[i].o_fileid = i;
		opentab[i].o_ff = (struct Filefd *)va;
		va += BY2PG;
	}
}

// Overview:
//	Allocate an open file.
int
open_alloc(struct Open **o)
{
	int i, r;

	// Find an available open-file table entry
	for (i = 0; i < MAXOPEN; i++) {
		switch (pageref(opentab[i].o_ff)) {
			case 0:
				if ((r = syscall_mem_alloc(0, (u_int)opentab[i].o_ff,
										   PTE_V | PTE_R | PTE_LIBRARY)) < 0) {
					return r;
				}
			case 1:
				opentab[i].o_fileid += MAXOPEN;
				*o = &opentab[i];
				user_bzero((void *)opentab[i].o_ff, BY2PG);
				return (*o)->o_fileid;
		}
	}

	return -E_MAX_OPEN;
}

// Overview:
//	Look up an open file for envid.
int
open_lookup(u_int envid, u_int fileid, struct Open **po)
{
	struct Open *o;

	o = &opentab[fileid % MAXOPEN];

	if (pageref(o->o_ff) == 1 || o->o_fileid != fileid) {
		return -E_INVAL;
	}

	*po = o;
	return 0;
}

static int
strncmp(char *a, char *b, int n) {
	int i;
	for (i = 0; i < n; i++) {
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

// Serve requests, sending responses back to envid.
// To send a result back, ipc_send(envid, r, 0, 0).
// To include a page, ipc_send(envid, r, srcva, perm).
// 此处按照路径转接到对应的文件系统
// root0表示自带的文件系统，root1表示FAT文件系统
void
serve_open(u_int envid, struct Fsreq_open *rq)
{
	writef("serve_open %08x %x 0x%x\n", envid, (int)rq->req_path, rq->req_omode);

	u_char path[MAXPATHLEN];
	struct File *f = NULL;
	struct Filefd *ff;
	int fileid;
	int r;
	struct Open *o;
	struct DIREnt *dirent = NULL;
	// 必要的初始化

	// Copy in the path, making sure it's null-terminated
	user_bcopy(rq->req_path, path, MAXPATHLEN);
	path[MAXPATHLEN - 1] = 0;

	if (strncmp(path, "/root0", 6) != 0 && strncmp(path, "/root1", 6) != 0) {
		// 不符合规则的文件名
		ipc_send(envid, -E_INVAL, 0, 0);
		return;
	}

	// 1. Find a file id.
	if ((r = open_alloc(&o)) < 0) {
		user_panic("open_alloc failed: %d, invalid path: %s", r, path);
		ipc_send(envid, r, 0, 0);
		return;
	}

	fileid = r;

	if (strncmp(path, "/root0", 6) == 0) {
		// writef("route to root0\n");
		// 自带文件系统
		// 2. Open the file.
		if ((r = file_open((char *)(path+6), &f)) < 0) { // 跳过前缀
		//	user_panic("file_open failed: %d, invalid path: %s", r, path);
			ipc_send(envid, r, 0, 0);
			return ;
		}

		// 3. Save the file pointer.
		o->o_file = f;
		o->fstype = 0; // 设定文件系统类型
	}
	else if (strncmp(path, "/root1", 6) == 0) {
		// FAT32文件系统
		if ((r = open_alloc(&o)) < 0) {
			user_panic("open_alloc failed: %d, invalid path: %s", r, path);
			ipc_send(envid, r, 0, 0);
			return;
		}

		fileid = r;

		if ((r = FAT_file_open((char *)(path+6), &dirent)) < 0) {
		//	user_panic("file_open failed: %d, invalid path: %s", r, path);
			ipc_send(envid, r, 0, 0);
			return;
		}

		// Save the file pointer.
		o->o_FATfile = dirent;
		o->fstype = 1; // 设定文件系统类型
	}

	// 4. Fill out the Filefd structure
	ff = (struct Filefd *)o->o_ff;
	if (f) ff->f_file = *f; // 在使用指针时一定注意判断指针是否为0
	ff->fstype = o->fstype;
	if (dirent) ff->f_FATfile = *dirent;
	ff->f_fileid = o->o_fileid;

	o->o_mode = rq->req_omode;
	ff->f_fd.fd_omode = o->o_mode;
	ff->f_fd.fd_dev_id = devfile.dev_id;

	ipc_send(envid, 0, (u_int)o->o_ff, PTE_V | PTE_R | PTE_LIBRARY);
}

void
serve_map(u_int envid, struct Fsreq_map *rq)
{

	struct Open *pOpen;

	u_int filebno;

	void *blk;

	int r;

	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	filebno = rq->req_offset / BY2BLK;

	// 根据文件系统的类型决定调用的函数
	if (pOpen->fstype == 0)
		r = file_get_block(pOpen->o_file, filebno, &blk);
	else if (pOpen->fstype == 1)
		r = FAT_file_get_block(pOpen->o_FATfile, filebno, &blk);
	else
		r = -E_NO_DISK;

	if (r < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	ipc_send(envid, 0, (u_int)blk, PTE_V | PTE_R | PTE_LIBRARY | PTE_FS); // 映射PTE_FS位
}

void
serve_set_size(u_int envid, struct Fsreq_set_size *rq)
{
	struct Open *pOpen;
	int r;
	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	if (pOpen->fstype == 0)
		r = file_set_size(pOpen->o_file, rq->req_size);
	else if (pOpen->fstype == 1)
		r = FAT_file_set_size(pOpen->o_FATfile, rq->req_size);
	else
		r = -E_NO_DISK;

	if (r < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	ipc_send(envid, 0, 0, 0);
}

void
serve_close(u_int envid, struct Fsreq_close *rq)
{
	struct Open *pOpen;

	int r;

	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	if (pOpen->fstype == 0)
		file_close(pOpen->o_file);
	else if (pOpen->fstype == 1) {
		FAT_file_close(pOpen->o_FATfile);
	}

	ipc_send(envid, 0, 0, 0);
}

// Overview:
//	fs service used to delete a file according path in `rq`.
/*** exercise 5.10 ***/
void
serve_remove(u_int envid, struct Fsreq_remove *rq)
{
	int r;
	u_char path[MAXPATHLEN];

	// Step 1: Copy in the path, making sure it's terminated.
	// Notice: add \0 to the tail of the path
	rq->req_path[MAXPATHLEN - 1] = '\0';
	strcpy(path, rq->req_path);

	if (strncmp(path, "/root0", 6) == 0) {
		// Step 2: Remove file from file system and response to user-level process.
		// Call file_remove and ipc_send an approprite value to corresponding env.
		r = file_remove(path+6);
	}
	else if (strncmp(path, "/root1", 6) == 0) {
		r = FAT_file_remove(path+6);
	}
	else {
		r = -E_INVAL;
	}
	ipc_send(envid, r, 0, 0);
}

void
serve_dirty(u_int envid, struct Fsreq_dirty *rq)
{

	// Your code here
	struct Open *pOpen;
	int r;

	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	// 根据文件系统的类型决定调用的函数
	if (pOpen->fstype == 0)
		r = file_dirty(pOpen->o_file, rq->req_offset);
	else if (pOpen->fstype == 1)
		r = FAT_file_dirty(pOpen->o_FATfile, rq->req_offset);
	else
		r = -E_NO_DISK;

	if (r < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}

	ipc_send(envid, 0, 0, 0);
}

// 两个文件系统同时同步
void
serve_sync(u_int envid)
{
	fs_sync();
	FAT_fs_sync();
	ipc_send(envid, 0, 0, 0);
}

void
serve(void)
{
	u_int req, whom, perm;

	for (;;) {
		perm = 0;

		req = ipc_recv(&whom, REQVA, &perm);


		// All requests must contain an argument page
		if (!(perm & PTE_V)) {
			writef("Invalid request from %08x: no argument page\n", whom);
			continue; // just leave it hanging, waiting for the next request.
		}

		switch (req) {
			case FSREQ_OPEN:
				serve_open(whom, (struct Fsreq_open *)REQVA);
				break;

			case FSREQ_MAP:
				serve_map(whom, (struct Fsreq_map *)REQVA);
				break;

			case FSREQ_SET_SIZE:
				serve_set_size(whom, (struct Fsreq_set_size *)REQVA);
				break;

			case FSREQ_CLOSE:
				serve_close(whom, (struct Fsreq_close *)REQVA);
				break;

			case FSREQ_DIRTY:
				serve_dirty(whom, (struct Fsreq_dirty *)REQVA);
				break;

			case FSREQ_REMOVE:
				serve_remove(whom, (struct Fsreq_remove *)REQVA);
				break;

			case FSREQ_SYNC:
				serve_sync(whom);
				break;

			default:
				writef("Invalid request code %d from %08x\n", whom, req);
				break;
		}

		syscall_mem_unmap(0, REQVA);
	}
}

void
umain(void)
{
	user_assert(sizeof(struct File) == BY2FILE);

	writef("FS is running\n");
	writef("FS can do I/O\n");

	serve_init();

	// 内置FS的初始化
	fs_init();
	fs_test();

	// FAT FS Initialization
	FAT_fs_init();

	serve();
}

