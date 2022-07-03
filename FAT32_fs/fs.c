/* 
 * fs.c: 操作文件系统的主模块
 * 主要功能是承担磁盘读写，文件处理等底层任务
 * 是连接磁盘层和文件系统层的桥梁。
 *
 */


#include "fs.h"
#include <mmu.h>

static struct BPB *bpb;
static int nblocks;
static struct DIREnt root;
static int __debug = 0;

static u_int *FATtable;
static u_int nFATtable;

#define MAXLONGENT 8
// 用于在查询文件名时存放长文件名项
typedef struct longEntSet {
	LongNameEnt *longEnt[MAXLONGENT];
	int cnt;
} longEntSet;

static void file_flush(struct DIREnt *);
static int block_is_free(u_int);
static int read_block(u_int blockno, void **blk, u_int *isnew);
static void write_block(u_int blockno);


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
		syscall_mem_alloc(0, va, PTE_V | PTE_R | PTE_FS);

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
	if (pages[pa>>12].blockCacheChanged != 0) {
		// writef("Write back dirty block: %d\n", blockno);
		pages[pa>>12].blockCacheChanged = 0;
	}

	// 重设权限位
	syscall_mem_map(0, va, 0, va, (PTE_V | PTE_R | PTE_LIBRARY | PTE_FS));
}

// -------------以下几个函数都涉及到块的管理（与FAT表有关系）------------

// Overview:
//	Check to see if the block 'blockno' is free via bitmap.
//
// Post-Condition:
//	Return 1 if the block is free, else 0.
static int
block_is_free(u_int blockno)
{
	if (bpb == 0 || blockno >= nblocks) {
		return 0;
	}

	if (FATtable[BLK2CLUS(blockno) - 2] == CLUS_FREE) {
		return 1;
	}

	return 0;
}

// Overview:
//	Mark a block as free in the bitmap.
/*** exercise 5.3 ***/
static void
free_block(u_int blockno)
{
	// Step 1: Check if the parameter `blockno` is valid (`blockno` can't be zero).
	if (bpb == 0 || blockno >= nblocks || blockno == 0)
		return;
	
	// Step 2: Update the flag bit in bitmap.
	// you can use bit operation to update flags, such as  a |= (1 << n) .
	FATtable[BLK2CLUS(blockno)-2] = CLUS_FREE;
}

// Overview:
//	Search in the bitmap for a free block and allocate it.
//
// Post-Condition:
//	Return block number allocated on success,
//		   -E_NO_DISK if we are out of blocks.
static int
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
static int
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

static void
load_root(void)
{
	// 分配空间，建立公用的rootDirEnt
    strcpy(root.DIR_Name, "/");
    root.DIR_Attr = ATTR_DIRECTORY;
    root.DIR_FileSize = 0;
    root.DIR_DstClusLO = bpb->BPB_RootClus & 0xffff;
    root.DIR_FstClusHI = (bpb->BPB_RootClus >> 16) & 0xffff;
}

static void
load_FATfs() {
	void *blk;
	int r, i;

	// 加载第0块
	if ((r = read_block(0, &blk, 0)) < 0) {
		user_panic("Can't read FAT's BPB Block!");
	}

	bpb = blk;

	// 检查bpb的内容是否合法
	// 1. 文件系统格式为FAT32
	if (strcmp(bpb->BS_FilSysType, "FAT32 ") != 0) {
		user_panic("FAT Error: not a valid FAT32 FS!");
	}

	// 2. 魔数为0x55, 0xAA
	if (bpb->Signature_word[0] != (char)0x55 || bpb->Signature_word[1] != (char)0xAA) {
		user_panic("FAT Error: not a valid FS!");
	}

	// 3. 检查磁盘容量是否超过总容量
	nblocks = bpb->BPB_TotSec32 / SECT2BLK;
	if (nblocks > FAT_DISKMAX / BY2BLK) {
		user_panic("FAT FS is too large to contain!");
	}
	int size = nblocks * BY2BLK / 1024 / 1024; //MB
	writef("[FAT size] Total: %dMB, ", size);

	// FAT表处理
	// 1. 加载FAT表
	for (i = 1; i <= 1 + NFATBLOCK; i++) {
		r = read_block(i, blk, 0);
		if (r < 0) {
			user_panic("\nError when read FAT table!");
		}
	}

	// 2. 计算已用容量
	nFATtable = bpb->BPB_FATSz32 * 512 / 4;
	FATtable = (u_int *)diskaddr(1);
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
	writef(">>>>>>>>>>>>>>>>>>>>  Initializing FAT FS(/root1)  <<<<<<<<<<<<<<<<<<\n");
	load_FATfs(); // 加载和检查bpb块和FAT表
	load_root(); // 将Root块加载到内存中
	check_write_block(); // 检查写磁盘块是否能正常工作
}

//---------------------------文件块管理底层-----------------------

static int getFileBlocks(u_int size) {
	if (size % BY2BLK == 0) {
		return size / BY2BLK;
	}
	else {
		return size / BY2BLK + 1;
	}
}

static int
countBlocks(struct DIREnt *file) {
	int clus = file->DIR_FstClusHI * 65536 + file->DIR_DstClusLO;
	int i = 0;
	if (clus == 0) return 0;
	// 如果文件不包含任何块，则直接返回0即可。
	else {
		while (clus != CLUS_FILEEND) {
			// if(__debug) writef("rolling! %d\n", clus);
			clus = FATtable[clus - 2];
			i += 1;
		}
		return i;
	}
}

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
// alloc为1时，表示: 为大于等于文件块数的块分配空间
// 检索文件的第pdiskbno个块，并将其块号保存到*pdiskbno中
static int
file_map_block(struct DIREnt *file, int filebno, u_int *pdiskbno, u_int alloc)
{
	int i;
	u_int clus;
	// 没想到又引入了新的bug？
	int nfileblk = countBlocks(file); // 此计算块数的方法同时适用于文件和目录
	// writef("filebno = %d[map_block].\n", filebno);

	// 查询文件的指定块块号，若alloc为1，则分配不存在的块
	if (filebno >= nfileblk) {
		// writef("[warning] alloc new file block! filebno = %d, nfileblk = %d\n", filebno, nfileblk);
		// 块不存在，需要进行分配
		if (filebno >= MAXFILESIZE / BY2BLK) { // 文件太大
			return -E_INVAL;
		}

		// 1. 定位到最后一块
		clus = file->DIR_FstClusHI * 65536 + file->DIR_DstClusLO;
		for (i = 0; i < nfileblk-1; i++) {
			clus = FATtable[clus-2];
		}

		// 2. 继续分配块
		for (i = 0; i < (filebno-nfileblk+1); i++) {
			int r = alloc_block();
			if (r < 0) {
				return r; // TODO: 考虑中途出现分配失败的情况，此时应当撤销之前的所有更改
			}
			
			if (clus == 0) { // 对应文件大小为0，所以初始块号为0的情况，先分配初始块
				file->DIR_FstClusHI = BLK2CLUS(r) >> 16; // 始终应当是簇号而不是块号
				file->DIR_DstClusLO = BLK2CLUS(r) & 0xffff;
			}
			else {
				// writef("Link a new block to %d!\n", BLK2CLUS(r));
				FATtable[clus-2] = BLK2CLUS(r);
			}
			clus = BLK2CLUS(r);
		}
		FATtable[clus-2] = CLUS_FILEEND;
		// 设置文件终止

		// writef("set pdiskbno...\n");
		*pdiskbno = CLUS2BLK(clus);
	}
	else {
		clus = file->DIR_FstClusHI * 65536 + file->DIR_DstClusLO;
		for (i = 0; i < filebno; i++) {
			// 一共进行filebno次迭代才能找到第filebno个块:filebno从0开始
			clus = FATtable[clus-2];
		}
		*pdiskbno = CLUS2BLK(clus);
	}

	return 0;
}

// Overview:
//	Remove a block from file f.  If it's not there, just silently succeed.
// 从链表上删除掉第filebno块
static int
file_clear_block(struct DIREnt *file, u_int filebno)
{
	int i;

	int nfileblk = countBlocks(file);
	if (filebno >= nfileblk) // 对应的块不存在
		return 0;

	// 此时不会出现文件块数为0的情况
	// 1.移动到第filebno块
	int clus = file->DIR_FstClusHI * 65536 + file->DIR_DstClusLO;
	int preClus = 0;
	for (i = 0; i < filebno; i++) {
		// 一共进行filebno次迭代才能找到第filebno个块:filebno从0开始
		preClus = clus;
		clus = FATtable[clus-2];
	}
	
	if (FATtable[clus-2] == CLUS_FILEEND) {
		if (preClus != 0) {
			// 说明不是是第一块(第一块什么也不做)
			FATtable[preClus-2] = CLUS_FILEEND;
			free_block(CLUS2BLK(clus));
		}
	}
	else {
		if (preClus != 0) {
			FATtable[preClus-2] = FATtable[clus-2];
		}
		else {
			file->DIR_FstClusHI = FATtable[clus-2] >> 16;
			file->DIR_DstClusLO = FATtable[clus-2] & 0xffff;
		}
		free_block(CLUS2BLK(clus));
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
FAT_file_get_block(struct DIREnt *f, u_int filebno, void **blk)
{
	int r;
	u_int diskbno;
	u_int isnew;

	// writef("filebno = %d[get_block]\n", filebno);
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

// ----------------------目录/文件路径逻辑------------------

// Overview:
//	Mark the offset/BY2BLK'th block dirty in file f by writing its first word to itself.
int
FAT_file_dirty(struct DIREnt *f, u_int offset)
{
	int r;
	void *blk;

	if ((r = FAT_file_get_block(f, offset / BY2BLK, &blk)) < 0) {
		return r;
	}

	*(volatile char *)blk = *(volatile char *)blk;
	return 0;
}

// 在字符串前面插入字符串
static void
strins(char *buf, char *str, int len) {
	int lbuf = strlen(buf);
	int i;
	for (i = lbuf; i >= 0; i--) {
		buf[i+len] = buf[i];
	}
	for (i = 0; i < len; i++) {
		buf[i] = str[i];
	}
}

// 从此开始！！
// Overview:
//	Try to find a file named "name" in dir.  If so, set *file to it.
//
// Post-Condition:
//	return 0 on success, and set the pointer to the target file in `*file`.
//		< 0 on error.
/*** exercise 5.7 ***/
// 从此开始支持长文件名
// 找一个名字为name的文件
static int
dir_lookup(struct DIREnt *dir, char *name, struct DIREnt **file, longEntSet *longSet)
{
	int r;
	u_int i, j;
	void *blk;
	struct DIREnt *f;
	struct LongNameEnt *longEnt;

	char tmp_name[MAXNAMELEN];
	tmp_name[0] = 0; // 初始化为空字符串

	longSet->cnt = 0; // 初始化longSet有0个元素
	
	int clus = dir->DIR_FstClusHI * 65536 + dir->DIR_DstClusLO;

	for (i = clus; i != CLUS_FILEEND; i = FATtable[i-2]) {
		// Step 2: Read the i'th block of the dir.
		// Hint: Use file_get_block.
		r = read_block(CLUS2BLK(i), &blk, 0);
		if (r < 0) return r;

		// Step 3: Find target file by file name in all files on this block.
		// If we find the target file, set the result to *file and set f_dir field.
		for (j = 0; j < BY2BLK / BY2DIRENT; j++) {
			f = ((struct DIREnt *)blk) + j;
			
			// 跳过空项
			if (f->DIR_Name[0] == 0 || f->DIR_Name[0] == 0xE5) continue;

			if (f->DIR_Attr == ATTR_LONG_NAME_MASK) {

				longEnt = (LongNameEnt *)f;
				// 是第一项
				if (longEnt->LDIR_Ord & LAST_LONG_ENTRY) {
					tmp_name[0] = 0;
					longSet->cnt = 0;
				}

				// 向longSet里面存放长文件名项的指针
				longSet->longEnt[longSet->cnt++] = longEnt;
				strins(tmp_name, longEnt->LDIR_Name3, 4);
				strins(tmp_name, longEnt->LDIR_Name2, 12);
				strins(tmp_name, longEnt->LDIR_Name1, 10);
			}
			else {
				strins(tmp_name, f->DIR_Name, 11);
				// writef("Find long name Ent: %s\n", tmp_name);
				// writef("size = %d\n", sizeof(LongNameEnt));
				if (strcmp(tmp_name, name) == 0) {
					*file = f;
					return 0;
				}
				else {
					tmp_name[0] = 0;
					longSet->cnt = 0;
				}
			}
		}

	}

	return -E_NOT_FOUND;
}


// Overview:
//	Alloc a new File structure under specified directory. Set *file
//	to point at a free File structure in dir.
// 需支持长文件名
static int
dir_alloc_Ent(struct DIREnt *dir, struct DIREnt **ent)
{
	int r;
	u_int i, j, lastClus = 0;
	void *blk;
	struct DIREnt *f;

	int clus = dir->DIR_FstClusHI * 65536 + dir->DIR_DstClusLO;

	if (__debug) writef("alloc!\n");

	for (i = clus; i != CLUS_FILEEND; i = FATtable[i-2]) {
		// Step 2: Read the i'th block of the dir.
		// Hint: Use file_get_block.
		r = read_block(CLUS2BLK(i), &blk, 0);
		if (r < 0) return r;

		// Step 3: Find target file by file name in all files on this block.
		// If we find the target file, set the result to *file and set f_dir field.
		for (j = 0; j < BY2BLK / BY2DIRENT; j++) {
			if (__debug) writef("j = %d\n", j);
			f = ((struct DIREnt *)blk) + j;

			// 文件名第一位为0，表示目录项空闲
			if (f->DIR_Name[0] == 0) {
				*ent = f;
				return 0;
			}
		}
		lastClus = i;
	}

	// 没有找到空闲的目录项，需要重新分配一个
	r = alloc_block();
	if (r < 0) return r;
	FATtable[lastClus-2] = BLK2CLUS(r);
	FATtable[BLK2CLUS(r)-2] = CLUS_FILEEND;

	if (__debug) writef("alloc!\n");

	// 从新分配的块中提取第一项作为返回值
	u_int isnew;
	r = read_block(r, &blk, &isnew);
	if (r < 0) return r;
	f = blk;
	*ent = f;

	return 0;
}

static void
mstrncpy(char *dst, char *src, int n) {
    int i;
    for (i = 0; i < n; i++) {
        // 为结束位
        if (src[i] == 0) {
            dst[i] = src[i];
            break;
        }
        else {
            dst[i] = src[i];
        }
    }
}

static int
dir_alloc_file(struct DIREnt *dir, struct DIREnt **file, char *name) {
	int len = strlen(name) + 1;
	int nlongEnt, i, index;
	struct DIREnt *ent;
	struct LongNameEnt *longEnt;
	if (len <= 11) {
		nlongEnt = 0;
	}
	else {
		nlongEnt = ((len-11) % 26 == 0) ? ((len-11) / 26) : ((len-11) / 26 + 1);
	}
	for (i = nlongEnt; i >= 1; i--) {
		dir_alloc_Ent(dir, &ent);
		longEnt = (LongNameEnt *)ent;

		if (i == nlongEnt) {
            longEnt->LDIR_Ord = LAST_LONG_ENTRY | i;
        } else {
            longEnt->LDIR_Ord = i;
        }
        longEnt->LDIR_Attr = ATTR_LONG_NAME_MASK;
        // 其它为0的项不用填，因为全局变量在加载时就自动清零了
        index = 11 + 26*(i-1);
        if (index <= len) mstrncpy(longEnt->LDIR_Name1, name+index, 10);
        if (index + 10 <= len) mstrncpy(longEnt->LDIR_Name2, name+index+10, 12);
        if (index + 22 <= len) mstrncpy(longEnt->LDIR_Name3, name+index+22, 4);
	}
	dir_alloc_Ent(dir, &ent);
	mstrncpy(ent->DIR_Name, name, 11);
	*file = ent;
}

// Overview:
//	Skip over slashes.
static char *
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
// longSet返回文件的长文件名信息
static int
walk_path(char *path, struct DIREnt **pdir, struct DIREnt **pfile, 
			char *lastelem, longEntSet *longSet)
{
	char *p;
	char name[MAXNAMELEN];
	struct DIREnt *dir, *file;
	int r;

	// start at the root.
	path = skip_slash(path);
	file = &root;
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

		if ((dir->DIR_Attr & ATTR_ARCHIVE) == 1) {
			return -E_NOT_FOUND;
		}

		if ((r = dir_lookup(dir, name, &file, longSet)) < 0) {
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

// --------------------- 文件逻辑 -------------------

// Overview:
//	Open "path".
//
// Post-Condition:
//	On success set *pfile to point at the file and return (返回文件或目录所占用的磁盘块数).
//	On error return < 0.
int
FAT_file_open(char *path, struct DIREnt **file)
{	
	longEntSet longSet;
	if(__debug) writef("prepare to open path: %s\n", path);
	int r = walk_path(path, 0, file, 0, &longSet);
	if (r < 0) return r;
	else {
		// writef("open path succeed!\n");
		return countBlocks(*file);
	}
}

// Overview:
//	Create "path".
//
// Post-Condition:
//	On success set *file to point at the file and return 0.
// 	On error return < 0.
char _name[MAXPATHLEN];
int
FAT_file_create(char *path, struct DIREnt **file)
{
	int r;
	struct DIREnt *dir, *f;
	longEntSet longSet;

	if ((r = walk_path(path, &dir, &f, _name, &longSet)) == 0) {
		return -E_FILE_EXISTS;
	}
	
	if (__debug) writef("walk path done!");
	if (r != -E_NOT_FOUND || dir == 0) {
		return r;
	}
	
	if (dir_alloc_file(dir, &f, _name) < 0) {
		return r;
	}
	
	if (__debug) writef("[#] Alloc a file!!\n");
	f->DIR_FileSize = 0;
	f->DIR_FstClusHI = f->DIR_DstClusLO = 0;
	*file = f;
	if (__debug) writef("[#] Alloc a file!! name = %s, len = %d\n", _name, strlen(_name));
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
static void
file_truncate(struct DIREnt *f, u_int newsize)
{
	u_int bno, old_nblocks, new_nblocks;

	old_nblocks = countBlocks(f);
	new_nblocks = getFileBlocks(newsize);

	// 相比于索引式，流程大大简化
	for (bno = new_nblocks; bno < old_nblocks; bno++) {
		file_clear_block(f, bno);
	}

	f->DIR_FileSize = newsize;
}

// Overview:
//	Set file size to newsize.
int
FAT_file_set_size(struct DIREnt *f, u_int newsize)
{
	if (f->DIR_FileSize > newsize) {
		file_truncate(f, newsize);
	}

	// 若newsize > f_size，会自动扩张，
	// 之后map时会通过file_map_block为新增的空间分配新块
	f->DIR_FileSize = newsize;

	// 暂时不能刷新目录文件，只能通过用户进程最后调用sync来实现内容的同步
	return 0;
}

// Overview:
//	Flush the contents of file f out to disk.
// 	Loop over all the blocks in file.
// 	Translate the file block number into a disk block number and then
//	check whether that disk block is dirty.  If so, write it out.
//
// Hint: use file_map_block, block_is_dirty, and write_block.
static void
file_flush(struct DIREnt *f)
{
	// Your code here
	u_int nblocks;
	u_int bno;
	u_int diskno;
	int r;

	nblocks = countBlocks(f);

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
FAT_fs_sync(void)
{
	writef("Sync changes into disk...(nblocks = %d)\nDirty block: ", nblocks);
	int i;
	for (i = 0; i < nblocks; i++) {
		if (block_is_dirty(i)) {
			writef("%d ", i);
			write_block(i);
		}
	}
	writef("\n");
}

// Overview:
//	Close a file.
void
FAT_file_close(struct DIREnt *f)
{
	file_flush(f);
}

// Overview:
//	Remove a file by truncating it and then zeroing the name.
// 需支持长文件名
int
FAT_file_remove(char *path)
{
	int r, i;
	struct DIREnt *f;
	longEntSet longSet;

	// Step 1: find the file on the disk.
	if ((r = walk_path(path, 0, &f, 0, &longSet)) < 0) {
		return r;
	}

	// Step 2: truncate it's size to zero.
	file_truncate(f, 0);

	// Step 3: clear it's name.
	f->DIR_Name[0] = 0xE5; // 需要全部设为0xE5
	for (i = 0; i < longSet.cnt; i++) {
		longSet.longEnt[i]->LDIR_Ord = 0xE5;
	}

	// Step 4: flush the file.
	file_flush(f);
	return 0;
}

