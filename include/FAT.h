#ifndef _FAT_H_
#define _FAT_H_
#include "types.h"

void FAT_fs_init();
void FAT_fs_sync();
int FAT_file_remove(char *path);
void FAT_file_close(struct DIREnt *f);
int FAT_file_dirty(struct DIREnt *f, u_int offset);
int FAT_file_set_size(struct DIREnt *f, u_int newsize);
int FAT_file_create(char *path, struct DIREnt **file);
int FAT_file_get_block(struct DIREnt *f, u_int filebno, void **blk);
int FAT_file_open(char *path, struct DIREnt **file);

#endif // !_FAT_H_