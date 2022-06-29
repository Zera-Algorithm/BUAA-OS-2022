/* 
 * fs.c: 操作文件系统的主模块
 * 主要功能是承担磁盘读写，文件处理等底层任务
 * 是连接磁盘层和文件系统层的桥梁。
 *
 */


#include "fs.h"
#include <mmu.h>

struct BPB *bpb;
int nblocks;
struct DIREnt root;

u_int *FATtable;
u_int nFATtable;

void file_flush(struct File *);
int block_is_free(u_int);

// ------------- 以下几个函数涉及块缓存的映射 -----------

// Overview:
//	Return the virtual address of this disk block. If the `blockno` is greater
//	than disk's nblocks, panic.
/*** exercise 5.5 ***/
static u_int
diskaddr(u_int blockno)
{
	/* Step1: check if blockno >= total blocks. */
	// Not necessary to check super.
	if (bpb && blockno >= nblocks) {
		user_panic("Blockno >= nblocks! The block can't be read.");
	}
	
	/* Step2: Calc the corresponding address. */
	return FAT_DISKMAP + BY2BLK * blockno;
}

// Overview:
//	Check if this virtual address is mapped to a block. (check PTE_V bit)
static u_int
va_is_mapped(u_int va)
{
	return (((*vpd)[PDX(va)] & (PTE_V)) && ((*vpt)[VPN(va)] & (PTE_V)));
}

// Overview:
//	Check if this disk block is mapped to a vitrual memory address. (check corresponding `va`)
static u_int
block_is_mapped(u_int blockno)
{
	u_int va = diskaddr(blockno);
	if (va_is_mapped(va)) {
		return va;
	}
	return 0;
}

// Overview:
//	Check if this virtual address is dirty. (check PTE_D bit)
static u_int
va_is_dirty(u_int va)
{
	u_long pa;
	pa = PTE_ADDR((*vpt)[va>>12]);
	return pages[pa>>12].blockCacheChanged;
	// return (* vpt)[VPN(va)] & PTE_D;
}

// Overview:
//	Check if this block is dirty. (check corresponding `va`)
static u_int
block_is_dirty(u_int blockno)
{
	u_int va = diskaddr(blockno);
	return va_is_mapped(va) && va_is_dirty(va);
}

// Overview:
//	Allocate a page to hold the disk block.
//
// Post-Condition:
//	If this block is already mapped to a virtual address(use `block_is_mapped`),
// 	then return 0, indicate success, else alloc a page for this `va` address,
//	and return the result(success or fail) of `syscall_mem_alloc`.
/*** exercise 5.6 ***/
static int
map_block(u_int blockno)
{
	int r;
	u_long pa;
	// Step 1: Decide whether this block has already mapped to a page of physical memory.
	if(block_is_mapped(blockno)) return 0;

	// Step 2: Alloc a page of memory for this block via syscall.
	r = syscall_mem_alloc(syscall_getenvid(), diskaddr(blockno), PTE_V | PTE_R | PTE_FS);
	pa = PTE_ADDR((*vpt)[diskaddr(blockno)>>12]);
	pages[pa>>12].blockCacheChanged = 0;
	return r;
}

// Overview:
//	Unmap a block.
/*** exercise 5.6 ***/
static void
unmap_block(u_int blockno)
{
	int r;
	u_long pa;

	// Step 1: check if this block is mapped.
	if (block_is_mapped(blockno) == 0) {
		// not mapped.
		return;
	}

	// Step 2: use block_is_free，block_is_dirty to check block,
	// if this block is used(not free) and dirty, it needs to be synced to disk: write_block
	// can't be unmap directly.
	if (!block_is_free(blockno) && block_is_dirty(blockno)) {
		write_block(blockno);
	}

	pa = PTE_ADDR((*vpt)[diskaddr(blockno)>>12]);
	pages[pa>>12].blockCacheChanged = 0;

	// Step 3: use 'syscall_mem_unmap' to unmap corresponding virtual memory.
	syscall_mem_unmap(0, diskaddr(blockno));

	// Step 4: validate result of this unmap operation.
	user_assert(!block_is_mapped(blockno));
}

// ------------------块读写------------------

// Overview:
//	Make sure a particular disk block is loaded into memory.
//
// Post-Condition:
//	Return 0 on success, or a negative error code on error.
//
//	If blk!=0, set *blk to the address of the block in memory.
//
// 	If isnew!=0, set *isnew to 0 if the block was already in memory, or
//	to 1 if the block was loaded off disk to satisfy this request. (Isnew
//	lets callers like file_get_block clear any memory-only fields
//	from the disk blocks when they come in off disk.)
//
// Hint:
//	use diskaddr, block_is_mapped, syscall_mem_alloc, and ide_read.
static int
read_block(u_int blockno, void **blk, u_int *isnew)
{
	u_int va;

	// Step 1: validate blockno. Make file the block to read is within the disk.
	if (bpb && blockno >= nblocks) {
		user_panic("reading non-existent block %08x\n", blockno);
	}

	// Step 2: validate this block is used, not free.
	// Hint:
	//	If the bitmap is NULL, indicate that we haven't read bitmap from disk to memory
	// 	until now. So, before we check if a block is free using `block_is_free`, we must
	// 	ensure that the bitmap blocks are already read from the disk to memory.
	if (FATtable && block_is_free(blockno)) {
		user_panic("reading free block %08x\n", blockno);
	}

	// Step 3: transform block number to corresponding virtual address.
	va = diskaddr(blockno);

	// Step 4: read disk and set *isnew.
	// Hint: 
	//  If this block is already mapped, just set *isnew, else alloc memory and
	//  read data from IDE disk (use `syscall_mem_alloc` and `ide_read`).
	//  We have only one IDE disk, so the diskno of ide_read should be 0.
	if (block_is_mapped(blockno)) {	// the block is in memory
		if (isnew) {
			*isnew = 0;
		}
	} else {			// the block is not in memory
		if (isnew) {
			*isnew = 1;
		}
		syscall_mem_alloc(0, va, PTE_V | PTE_R);

		// 我们文件系统默认挂载在1号盘
		ide_read(1, blockno * SECT2BLK, (void *)va, SECT2BLK);
	}

	// Step 5: if blk != NULL, set `va` to *blk.
	if (blk) {
		*blk = (void *)va;
	}
	return 0;
}

// Overview:
//	Wirte the current contents of the block out to disk.
static void
write_block(u_int blockno)
{
	u_int va;
	u_long pa;

	// Step 1: detect is this block is mapped, if not, can't write it's data to disk.
	if (!block_is_mapped(blockno)) {
		user_panic("write unmapped block %08x", blockno);
	}

	// Step2: write data to IDE disk. (using ide_write, and the diskno is 0)
	va = diskaddr(blockno);
	ide_write(1, blockno * SECT2BLK, (void *)va, SECT2BLK);

	// 清空blockCacheChanged字段，防止再次出现页写入异常
	pa = PTE_ADDR((*vpt)[diskaddr(blockno)>>12]);
	pages[pa>>12].blockCacheChanged = 0;

	// 重设权限位
	syscall_mem_map(0, va, 0, va, (PTE_V | PTE_R | PTE_LIBRARY | PTE_FS));
}

// -------------以下几个函数都涉及到块的管理（与FAT表有关系）------------

// Overview:
//	Check to see if the block 'blockno' is free via bitmap.
//
// Post-Condition:
//	Return 1 if the block is free, else 0.
int
block_is_free(u_int blockno)
{
	if (bpb == 0 || blockno >= nblocks) {
		return 0;
	}

	if (FATtable[BLK2CLUS(blockno)] == CLUS_FREE) {
		return 1;
	}

	return 0;
}

// Overview:
//	Mark a block as free in the bitmap.
/*** exercise 5.3 ***/
void
free_block(u_int blockno)
{
	// Step 1: Check if the parameter `blockno` is valid (`blockno` can't be zero).
	if (bpb == 0 || blockno >= nblocks || blockno == 0)
		return;
	
	// Step 2: Update the flag bit in bitmap.
	// you can use bit operation to update flags, such as  a |= (1 << n) .
	FATtable[BLK2CLUS(blockno)] = CLUS_FREE;
}

// Overview:
//	Search in the bitmap for a free block and allocate it.
//
// Post-Condition:
//	Return block number allocated on success,
//		   -E_NO_DISK if we are out of blocks.
int
alloc_block_num(void)
{
	int i;
	// 遍历FAT表，找到第一个FREE的表项
	for (i = 0; i < (NFATBLOCK * BY2BLK / 4); i++) {
		// i表示表项数，CLUS=i+2
		if (FATtable[i] == CLUS_FREE) {
			// 现在无法填写表项，只能先分配出去
			return CLUS2BLK(i+2);
		}
	}
	// no free blocks.
	return -E_NO_DISK;
}

// Overview:
//	Allocate a block -- first find a free block in the bitmap, then map it into memory.
int
alloc_block(void)
{
	int r, bno;
	// Step 1: find a free block.
	if ((r = alloc_block_num()) < 0) { // failed.
		return r;
	}
	bno = r;

	// Step 2: map this block into memory.
	if ((r = map_block(bno)) < 0) {
		free_block(bno);
		return r;
	}

	// Step 3: return block number.
	return bno;
}

//--------------------------文件系统初始化------------------------

// Overview:
//	Test that write_block works, by smashing the superblock and reading it back.
// 检查写磁盘是否能正常工作
static void
check_write_block(void)
{

}

void
load_root(void)
{
	// 分配空间，建立公用的rootDirEnt
    strcpy(root.DIR_Name, "/");
    root.DIR_Attr = ATTR_DIRECTORY;
    root.DIR_FileSize = 0;
    root.DIR_DstClusLO = bpb->BPB_RootClus & 0xffff;
    root.DIR_FstClusHI = (bpb->BPB_RootClus >> 16) & 0xffff;
}

void load_FATfs() {
	void *blk;
	int r, i;

	// 加载第0块
	if ((r = read_block(0, blk, 0)) < 0) {
		user_panic("Can't read FAT's BPB Block!");
	}

	bpb = blk;

	// 检查bpb的内容是否合法
	// 1. 文件系统格式为FAT32
	if (strcmp(bpb->BS_FilSysType, "FAT32 ") != 0) {
		user_panic("FAT Error: not a valid FAT32 FS!");
	}

	// 2. 魔数为0x55, 0xAA
	if (bpb->Signature_word[0] != 0x55 || bpb->Signature_word[1] != 0xAA) {
		user_panic("FAT Error: not a valid FS!");
	}

	// 3. 检查磁盘容量是否超过总容量
	nblocks = bpb->BPB_TotSec32 / SECT2BLK;
	if (nblocks > FAT_DISKMAX / BY2BLK) {
		user_panic("FAT FS is too large to contain!");
	}
	int size = nblocks * BY2BLK / 1024 / 1024; //MB
	writef("FAT size:\nTotal: %dMB, ", size);

	// FAT表处理
	// 1. 加载FAT表
	for (i = 1; i <= 1 + NFATBLOCK; i++) {
		r = read_block(i, blk, 0);
		if (r < 0) {
			user_panic("\nError when read FAT table!");
		}
	}

	// 2. 计算已用容量
	nFATtable = bpb->BPB_FATSz32;
	int usedFATent = 0;
	for (i = 0; i < nFATtable; i++) {
		if (FATtable[i] != CLUS_FREE) {
			usedFATent += 1;
		}
	}
	size = (usedFATent + NFATBLOCK + 1) * BY2BLK / 1024;
	writef("Used: %dKB\n", size);

	writef("[OK] FAT FS Check passed.\n");
}

// Overview:
//	Initialize the file system.
// Hint:
//	1. read super block.
//	2. check if the disk can work.
//	3. read bitmap blocks from disk to memory.
void
FAT_fs_init(void)
{
	load_FATfs(); // 加载和检查bpb块和FAT表
	load_root(); // 将Root块加载到内存中
	check_write_block(); // 检查写磁盘块是否能正常工作
}

//---------------------------文件管理逻辑-----------------------

// Overview:
//	Like pgdir_walk but for files.
//	Find the disk block number slot for the 'filebno'th block in file 'f'. Then, set
//	'*ppdiskbno' to point to that slot. The slot will be one of the f->f_direct[] entries,
// 	or an entry in the indirect block.
// 	When 'alloc' is set, this function will allocate an indirect block if necessary.
//
// Post-Condition:
//	Return 0: success, and set the pointer to the target block in *ppdiskbno(Note that the pointer
//			might be NULL).
//		-E_NOT_FOUND if the function needed to allocate an indirect block, but alloc was 0.
//		-E_NO_DISK if there's no space on the disk for an indirect block.
//		-E_NO_MEM if there's no space in memory for an indirect block.
//		-E_INVAL if filebno is out of range (it's >= NINDIRECT).
int
file_block_walk(struct File *f, u_int filebno, u_int **ppdiskbno, u_int alloc)
{
	int r;
	u_int *ptr;
	void *blk;

	if (filebno < NDIRECT) {
		// Step 1: if the target block is corresponded to a direct pointer, just return the
		// 	disk block number.
		ptr = &f->f_direct[filebno];
	} else if (filebno < NINDIRECT) {
		// Step 2: if the target block is corresponded to the indirect block, but there's no
		//	indirect block and `alloc` is set, create the indirect block.
		if (f->f_indirect == 0) {
			if (alloc == 0) {
				return -E_NOT_FOUND;
			}

			if ((r = alloc_block()) < 0) {
				return r;
			}
			f->f_indirect = r;
		}

		// Step 3: read the new indirect block to memory.
		if ((r = read_block(f->f_indirect, &blk, 0)) < 0) {
			return r;
		}
		ptr = (u_int *)blk + filebno;
	} else {
		return -E_INVAL;
	}

	// Step 4: store the result into *ppdiskbno, and return 0.
	*ppdiskbno = ptr;
	return 0;
}

// OVerview:
//	Set *diskbno to the disk block number for the filebno'th block in file f.
// 	If alloc is set and the block does not exist, allocate it.
//
// Post-Condition:
// 	Returns 0: success, < 0 on error.
//	Errors are:
//		-E_NOT_FOUND: alloc was 0 but the block did not exist.
//		-E_NO_DISK: if a block needed to be allocated but the disk is full.
//		-E_NO_MEM: if we're out of memory.
//		-E_INVAL: if filebno is out of range.
int
file_map_block(struct File *f, u_int filebno, u_int *diskbno, u_int alloc)
{
	int r;
	u_int *ptr;

	// Step 1: find the pointer for the target block.
	if ((r = file_block_walk(f, filebno, &ptr, alloc)) < 0) {
		return r;
	}

	// Step 2: if the block not exists, and create is set, alloc one.
	if (*ptr == 0) {
		if (alloc == 0) {
			return -E_NOT_FOUND;
		}

		if ((r = alloc_block()) < 0) {
			return r;
		}
		*ptr = r;
	}

	// Step 3: set the pointer to the block in *diskbno and return 0.
	*diskbno = *ptr;
	return 0;
}

// Overview:
//	Remove a block from file f.  If it's not there, just silently succeed.
int
file_clear_block(struct File *f, u_int filebno)
{
	int r;
	u_int *ptr;

	if ((r = file_block_walk(f, filebno, &ptr, 0)) < 0) {
		return r;
	}

	if (*ptr) {
		free_block(*ptr);
		*ptr = 0;
	}

	return 0;
}

// Overview:
//	Set *blk to point at the filebno'th block in file f.
//
// Hint: use file_map_block and read_block.
//
// Post-Condition:
//	return 0 on success, and read the data to `blk`, return <0 on error.
int
file_get_block(struct File *f, u_int filebno, void **blk)
{
	int r;
	u_int diskbno;
	u_int isnew;

	// Step 1: find the disk block number is `f` using `file_map_block`.
	if ((r = file_map_block(f, filebno, &diskbno, 1)) < 0) {
		return r;
	}

	// Step 2: read the data in this disk to blk.
	if ((r = read_block(diskbno, blk, &isnew)) < 0) {
		return r;
	}
	return 0;
}

// Overview:
//	Mark the offset/BY2BLK'th block dirty in file f by writing its first word to itself.
int
file_dirty(struct File *f, u_int offset)
{
	int r;
	void *blk;

	if ((r = file_get_block(f, offset / BY2BLK, &blk)) < 0) {
		return r;
	}

	*(volatile char *)blk = *(volatile char *)blk;
	return 0;
}

// Overview:
//	Try to find a file named "name" in dir.  If so, set *file to it.
//
// Post-Condition:
//	return 0 on success, and set the pointer to the target file in `*file`.
//		< 0 on error.
/*** exercise 5.7 ***/
int
dir_lookup(struct File *dir, char *name, struct File **file)
{
	int r;
	u_int i, j, nblock;
	void *blk;
	struct File *f;

	// Step 1: Calculate nblock: how many blocks are there in this dir?
	if (dir->f_size % BY2BLK == 0) {
		nblock = dir->f_size / BY2BLK;
	}
	else {
		nblock = dir->f_size / BY2BLK + 1;
	}

	for (i = 0; i < nblock; i++) {
		// Step 2: Read the i'th block of the dir.
		// Hint: Use file_get_block.
		r = file_get_block(dir, i, &blk);
		if (r < 0) return r;

		// Step 3: Find target file by file name in all files on this block.
		// If we find the target file, set the result to *file and set f_dir field.
		for (j = 0; j < BY2BLK / BY2FILE; j++) {
			f = ((struct File *)blk) + j;
			if (strcmp(f->f_name, name) == 0) {
				f->f_dir = dir;
				*file = f;
				return 0;
			}
		}

	}

	return -E_NOT_FOUND;
}


// Overview:
//	Alloc a new File structure under specified directory. Set *file
//	to point at a free File structure in dir.
int
dir_alloc_file(struct File *dir, struct File **file)
{
	int r;
	u_int nblock, i, j;
	void *blk;
	struct File *f;

	nblock = dir->f_size / BY2BLK;

	for (i = 0; i < nblock; i++) {
		// read the block.
		if ((r = file_get_block(dir, i, &blk)) < 0) {
			return r;
		}

		f = blk;

		for (j = 0; j < FILE2BLK; j++) {
			if (f[j].f_name[0] == '\0') { // found free File structure.
				*file = &f[j];
				return 0;
			}
		}
	}

	// no free File structure in exists data block.
	// new data block need to be created.
	dir->f_size += BY2BLK;
	if ((r = file_get_block(dir, i, &blk)) < 0) {
		return r;
	}
	f = blk;
	*file = &f[0];

	return 0;
}

// Overview:
//	Skip over slashes.
char *
skip_slash(char *p)
{
	while (*p == '/') {
		p++;
	}
	return p;
}

// Overview:
//	Evaluate a path name, starting at the root.
//
// Post-Condition:
// 	On success, set *pfile to the file we found and set *pdir to the directory
//	the file is in.
//	If we cannot find the file but find the directory it should be in, set
//	*pdir and copy the final path element into lastelem.
int
walk_path(char *path, struct File **pdir, struct File **pfile, char *lastelem)
{
	char *p;
	char name[MAXNAMELEN];
	struct File *dir, *file;
	int r;

	// start at the root.
	path = skip_slash(path);
	file = &super->s_root;
	dir = 0;
	name[0] = 0;

	if (pdir) {
		*pdir = 0;
	}

	*pfile = 0;

	// find the target file by name recursively.
	while (*path != '\0') {
		dir = file;
		p = path;

		while (*path != '/' && *path != '\0') {
			path++;
		}

		if (path - p >= MAXNAMELEN) {
			return -E_BAD_PATH;
		}

		user_bcopy(p, name, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (dir->f_type != FTYPE_DIR) {
			return -E_NOT_FOUND;
		}

		if ((r = dir_lookup(dir, name, &file)) < 0) {
			if (r == -E_NOT_FOUND && *path == '\0') {
				if (pdir) {
					*pdir = dir;
				}

				if (lastelem) {
					strcpy(lastelem, name);
				}

				*pfile = 0;
			}

			return r;
		}
	}

	if (pdir) {
		*pdir = dir;
	}

	*pfile = file;
	return 0;
}

// Overview:
//	Open "path".
//
// Post-Condition:
//	On success set *pfile to point at the file and return 0.
//	On error return < 0.
int
file_open(char *path, struct File **file)
{
	return walk_path(path, 0, file, 0);
}

// Overview:
//	Create "path".
//
// Post-Condition:
//	On success set *file to point at the file and return 0.
// 	On error return < 0.
int
file_create(char *path, struct File **file)
{
	char name[MAXNAMELEN];
	int r;
	struct File *dir, *f;

	if ((r = walk_path(path, &dir, &f, name)) == 0) {
		return -E_FILE_EXISTS;
	}

	if (r != -E_NOT_FOUND || dir == 0) {
		return r;
	}

	if (dir_alloc_file(dir, &f) < 0) {
		return r;
	}

	strcpy((char *)f->f_name, name);
	*file = f;
	return 0;
}

// Overview:
//	Truncate file down to newsize bytes.
// 	Since the file is shorter, we can free the blocks that were used by the old
//	bigger version but not by our new smaller self.  For both the old and new sizes,
// 	figure out the number of blocks required, and then clear the blocks from
//	new_nblocks to old_nblocks.
//
// 	If the new_nblocks is no more than NDIRECT, free the indirect block too.
//	(Remember to clear the f->f_indirect pointer so you'll know whether it's valid!)
//
// Hint: use file_clear_block.
void
file_truncate(struct File *f, u_int newsize)
{
	u_int bno, old_nblocks, new_nblocks;

	old_nblocks = f->f_size / BY2BLK + 1;
	new_nblocks = newsize / BY2BLK + 1;

	if (newsize == 0) {
		new_nblocks = 0;
	}

	if (new_nblocks <= NDIRECT) {
		for (bno = new_nblocks; bno < old_nblocks; bno++) {
			file_clear_block(f, bno);
		}
		if (f->f_indirect) {
			free_block(f->f_indirect);
			f->f_indirect = 0;
		}
	} else {
		for (bno = new_nblocks; bno < old_nblocks; bno++) {
			file_clear_block(f, bno);
		}
	}

	f->f_size = newsize;
}

// Overview:
//	Set file size to newsize.
int
file_set_size(struct File *f, u_int newsize)
{
	if (f->f_size > newsize) {
		file_truncate(f, newsize);
	}

	f->f_size = newsize;

	if (f->f_dir) {
		file_flush(f->f_dir);
	}

	return 0;
}

// Overview:
//	Flush the contents of file f out to disk.
// 	Loop over all the blocks in file.
// 	Translate the file block number into a disk block number and then
//	check whether that disk block is dirty.  If so, write it out.
//
// Hint: use file_map_block, block_is_dirty, and write_block.
void
file_flush(struct File *f)
{
	// Your code here
	u_int nblocks;
	u_int bno;
	u_int diskno;
	int r;

	nblocks = f->f_size / BY2BLK + 1;

	for (bno = 0; bno < nblocks; bno++) {
		if ((r = file_map_block(f, bno, &diskno, 0)) < 0) {
			continue;
		}
		if (block_is_dirty(diskno)) {
			write_block(diskno);
		}
	}
}

// Overview:
//	Sync the entire file system.  A big hammer.
void
fs_sync(void)
{
	int i;
	for (i = 0; i < super->s_nblocks; i++) {
		if (block_is_dirty(i)) {
			write_block(i);
		}
	}
}

// Overview:
//	Close a file.
void
file_close(struct File *f)
{
	// Flush the file itself, if f's f_dir is set, flush it's f_dir.
	file_flush(f);
	if (f->f_dir) {
		file_flush(f->f_dir);
	}
}

// Overview:
//	Remove a file by truncating it and then zeroing the name.
int
file_remove(char *path)
{
	int r;
	struct File *f;

	// Step 1: find the file on the disk.
	if ((r = walk_path(path, 0, &f, 0)) < 0) {
		return r;
	}

	// Step 2: truncate it's size to zero.
	file_truncate(f, 0);

	// Step 3: clear it's name.
	f->f_name[0] = '\0';

	// Step 4: flush the file.
	file_flush(f);
	if (f->f_dir) {
		file_flush(f->f_dir);
	}

	return 0;
}

