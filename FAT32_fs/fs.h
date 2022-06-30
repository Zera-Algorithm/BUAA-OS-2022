#include "lib.h"
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

