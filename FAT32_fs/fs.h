#include "lib.h"
#include <fs.h>
#include <mmu.h>
#include "fat32.h"

/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP+(n*BY2BLK). */
#define FAT_DISKMAP		0x30000000

/* Maximum disk size we can handle (0.5GB) */
#define FAT_DISKMAX		0x20000000

#define BY2SECT		512	/* Bytes per disk sector */
#define SECT2BLK	(BY2BLK/BY2SECT)	/* sectors to a block */

/* ide.c */
void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs);
void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs);

/* fs.c */
int file_open(char *path, struct File **pfile);
int file_get_block(struct File *f, u_int blockno, void **pblk);
int file_set_size(struct File *f, u_int newsize);
void file_close(struct File *f);
int file_remove(char *path);
int file_dirty(struct File *f, u_int offset);
void file_flush(struct File *);

void fs_init(void);
void fs_sync(void);
int map_block(u_int);
int alloc_block(void);