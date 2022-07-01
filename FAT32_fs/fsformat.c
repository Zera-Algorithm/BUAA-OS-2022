#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include "fat32.h"

#define MAXNAMELEN 1024

// 区分块的类型
enum BLOCK_TYPE {
    BLOCK_FREE  = 0,
    BLOCK_BPB   = 1,
    BLOCK_FAT   = 2,
    BLOCK_DIR   = 3,
    BLOCK_FILE  = 4,
};

struct Block {
    u_int8_t data[BY2BLK];
    u_int32_t type;
};

// 初始化均为FREE状态
struct Block blocks[NBLOCK];
int next_cluster = ROOTCLUS;
DIREnt *rootDirEnt;
struct FSInfo *fsinfo;
char buf[1025];
int isReverse = 1;

// 获取一个新的簇号（同时将最新的簇号加一，使得next_cluster是当前最小的没被使用过的簇）
int new_clus(enum CLUS_TYPE clus_type) {
    blocks[CLUS2BLK(next_cluster)].type = clus_type;
    return next_cluster++;
}

// 获取一个新的块号
int new_blk(enum CLUS_TYPE clus_type) {
    return CLUS2BLK(new_clus(clus_type));
}

void init_BPB(struct BPB *bpb) {
    strcpy(bpb->BS_OEMName, "MOS ");
    bpb->BPB_BytesPerSec    = 512;
    bpb->BPB_SecPerClus     = 8;
    bpb->BPB_RsvdSecCnt     = NBPBBLOCK * 8;
    bpb->BPB_NumFATs        = 1;
    bpb->BPB_Media          = 0xF8;
    bpb->BPB_TotSec32       = NBLOCK * 8;
    bpb->BPB_FATSz32        = NFATBLOCK * 8;
    bpb->BPB_ExtFlags       = 128;
    bpb->BPB_RootClus       = ROOTCLUS;
    bpb->BPB_FSInfo         = 1;
    bpb->BS_BootSig         = 0x29;
    bpb->BS_VolID           = 0x12a45e5b; // 自定义的卷号
    strcpy(bpb->BS_VolLab, "DATADISK "); // 卷标
    strcpy(bpb->BS_FilSysType, "FAT32 "); // 文件系统名
    bpb->Signature_word[0]  = 0x55;
    bpb->Signature_word[1]  = 0xAA;
}

void init_FSInfo(struct FSInfo *info) {
    info->FSI_LeadSig   = 0x41615252;
    info->FSI_StrucSig  = 0x61417272;
    info->FSI_TrailSig  = 0xAA550000;
}

uint *getpFATEnt(int N_clus) {
    assert(N_clus >= 2);
    int offset = (N_clus - 2) * 4;
    int n_block = offset / BY2BLK;
    return (uint *)(blocks[NBPBBLOCK + n_block].data + offset % BY2BLK);
}

void init_root() {
    // 为root分配簇（root目录项不包括.和..表项，所以初始时内容为空）
    uint clus = new_clus(BLOCK_DIR);
    uint *pEnt = getpFATEnt(clus);
    *pEnt = CLUS_FILEEND;

    // 分配空间，建立一个公用的rootDirEnt，供..项使用
    rootDirEnt = (DIREnt *)malloc(sizeof(DIREnt));
    strcpy(rootDirEnt->DIR_Name, "/");
    rootDirEnt->DIR_Attr = ATTR_DIRECTORY;
    rootDirEnt->DIR_FileSize = 0;
    rootDirEnt->DIR_DstClusLO = ROOTCLUS & 0xffff;
    rootDirEnt->DIR_FstClusHI = (ROOTCLUS >> 16) & 0xffff;
}

void init_disk() {
    int i;
    blocks[0].type = BLOCK_BPB;

    // 结构体占用空间打印
    // printf("BPB_size = %d\n", sizeof(struct BPB));
    // printf("DIREnt_size = %d, LongNameEnt_size = %d\n", 
    //         sizeof(struct DIREnt), sizeof(struct LongNameEnt));
    // printf("pointer_size = %d\n", sizeof(int *));

    assert(sizeof(struct BPB) == 512);
    assert(sizeof(struct DIREnt) == 32 && sizeof(struct LongNameEnt) == 32);

    init_BPB((struct BPB *)blocks[0].data);
    fsinfo = (struct FSInfo *)(blocks[0].data + 512);
    init_FSInfo(fsinfo);

    int FATBLK_START = NBPBBLOCK;
    for(i = 0; i < NFATBLOCK; i++) {
        blocks[FATBLK_START + i].type = BLOCK_FAT;
    }

    init_root();
}

// 长文件名项每项能存储26个字符
// 由于长文件名项最后一项必须要有0x40掩码，
// 所以最大长文件名项数为0x40-1 = 63，最长文件名为63 * 26 + 11 = 1649
// 不过为了方便管理，我们规定最长的文件名为1024


// 获取一个空的目录项位置
// 若当前目录空间已满，则为当前目录新分配一个簇
DIREnt *findEmptyEnt(uint DIRclus) {
    uint clus = DIRclus;
    int i;
    uint *pFAT;
    while(1) {
        DIREnt *pEnt = (DIREnt *)(blocks[CLUS2BLK(clus)].data);
        for (i = 0; i < BY2BLK / BY2DIRENT; i++) {
            if (pEnt[i].DIR_Name[0] == 0x00) {
                // 表示此表项以及之后各表项均为空
                printf("[1] find an Entry in CLUSTER #%d.\n", clus);
                return (pEnt + i);
            }
        }
        pFAT = getpFATEnt(clus);
        clus = *pFAT;
        if (clus == CLUS_FILEEND) {
            // 分配一个新块，并标记块是存储目录信息的块
            clus = new_clus(BLOCK_DIR);
            *pFAT = clus;
            *getpFATEnt(clus) = CLUS_FILEEND;

            printf("[2] find an Entry in CLUSTER #%d.\n", clus);
            return (DIREnt *)blocks[CLUS2BLK(clus)].data;
        }
    }
}

void mstrncpy(char *dst, char *src, int n) {
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

// 查询到一个存储文件的空位，存储文件名，并返回文件的目录项（长文件名项已经填好）
// 必须提前传入名称项，以分辨为此文件分配多少个长文件名项
DIREnt *create_file(int DIRclus, char *name) {
    int len = strlen(name);
    int n_longEnt;
    int i, index;
    if (len > MAXNAMELEN) {
        printf("Filename %s:\nToo long!!\n", name);
    }
    if ((len + 1) - 11 < 0) {
        n_longEnt = 0;
    }
    else {
        if(((len + 1) - 11) % CHAR2LONGENT == 0)
            n_longEnt = ((len + 1) - 11) / CHAR2LONGENT;
        else
            n_longEnt = ((len + 1) - 11) / CHAR2LONGENT + 1;
    }

    printf("file %s, n_longEnt = %d.\n", name, n_longEnt);
    // 复制文件名到longEnt
    for (i = n_longEnt; i >= 1; i--) {
        LongNameEnt *longEnt = (LongNameEnt *)findEmptyEnt(DIRclus);
        if (i == n_longEnt) {
            longEnt->LDIR_Ord = LAST_LONG_ENTRY | i;
        } else {
            longEnt->LDIR_Ord = i;
        }
        longEnt->LDIR_Attr = ATTR_LONG_NAME_MASK;
        // 其它为0的项不用填，因为全局变量在加载时就自动清零了
        index = 11 + 26*(i-1);
        if (index <= len) mstrncpy(longEnt->LDIR_Name1, name+index, 10);
        if (index + 10 <= len) mstrncpy(longEnt->LDIR_Name2, name+index+10, 12);
        if (index + 22 <= len) mstrncpy(longEnt->LDIR_Name2, name+index+22, 4);
    }

    DIREnt *dirEnt = findEmptyEnt(DIRclus);
    mstrncpy(dirEnt->DIR_Name, name, 11);
    return dirEnt;
}

// path是本机器上要写入文件的路径
void write_file(DIREnt *lastDir, const char *path) {
    int DIRclus = (lastDir->DIR_FstClusHI << 16) + lastDir->DIR_DstClusLO;
    int r, n_clus;

    // Get file name with no path prefix.
    // 找到path中最右面的/所在的字符指针位置，若无，返回null。
    // Sample: "/root/zrp/pic.jpg" -> "pic.jpg"
    char *fname = strrchr(path, '/');
    if(fname)
        fname++;
    else
        fname = path;

    DIREnt *dirEnt = create_file(DIRclus, fname);
    dirEnt->DIR_Attr = 0; // 文件的属性默认为可读可写
    
    int fd = open(path, O_RDONLY);
    dirEnt->DIR_FileSize = lseek(fd, 0, SEEK_END);

    lseek(fd, 0, SEEK_SET);// 将文件指针拨到文件开头
    n_clus = (dirEnt->DIR_FileSize % BY2CLUS == 0) ? (dirEnt->DIR_FileSize / BY2CLUS) 
                    : (dirEnt->DIR_FileSize / BY2CLUS + 1); // 计算所占用的簇数
    for (int i = 0; i < n_clus; i++) {
        read(fd, blocks[CLUS2BLK(next_cluster)].data, BY2BLK);
        if (i == 0) {
            // 为文件设置块号
            dirEnt->DIR_DstClusLO = next_cluster & 0xffff;
            dirEnt->DIR_FstClusHI = (next_cluster >> 16) & 0xffff;
        }
        
        // 不是最后一个簇
        if (i != n_clus - 1) {
            *getpFATEnt(next_cluster) = next_cluster + 1;
        }
        else {
            *getpFATEnt(next_cluster) = CLUS_FILEEND;
        }

        // 记录当前块的类型为文件块
        blocks[CLUS2BLK(next_cluster)].type = BLOCK_FILE;
        next_cluster += 1;
    }
    close(fd);
}


void write_directory(DIREnt *lastDir, const char *path) {
    int DIRclus = (lastDir->DIR_FstClusHI << 16) + lastDir->DIR_DstClusLO;
    int r, n_clus, firstClus;

    // Get file name with no path prefix.
    // 找到path中最右面的/所在的字符指针位置，若无，返回null。
    // Sample: "/root/zrp/pic.jpg" -> "pic.jpg"
    char *fname = strrchr(path, '/');
    if(fname)
        fname++;
    else
        fname = path;

    DIREnt *dirEnt = create_file(DIRclus, fname);
    dirEnt->DIR_Attr = ATTR_DIRECTORY; // 设置目录属性
    dirEnt->DIR_FileSize = 0; // 目录的size必须为0

    firstClus = new_clus(BLOCK_DIR);

    dirEnt->DIR_FstClusHI = (firstClus >> 16) & 0xffff;
    dirEnt->DIR_DstClusLO = firstClus & 0xffff;

    *getpFATEnt(firstClus) = CLUS_FILEEND;
    
    struct dirent *file;
    DIR *dp;
    DIREnt *myEnts = (DIREnt *)blocks[CLUS2BLK(firstClus)].data;

    // 首先为目录添加.和..两项
    memcpy((void *)myEnts, (void *)dirEnt, sizeof(DIREnt)); // 复制.项目
    strcpy(myEnts->DIR_Name, ".");

    myEnts += 1;
    memcpy((void *)myEnts, (void *)lastDir, sizeof(DIREnt));
    strcpy(myEnts->DIR_Name, "..");

    // 扫描目录
    while (file = readdir(dp))
    {
        sprintf(buf, "%s/%s", path, file->d_name); // 生成内部目录项的路径
        if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
            continue;
        }

        // 分文件和文件夹两种类型处理
        if (file->d_type == DT_DIR) {
            printf("Detected: folder: %s\n", file->d_name);
			write_directory(dirEnt, buf);
        }
        else {
            printf("Detected: file: %s\n", file->d_name);
			write_file(dirEnt, buf);
        }
    }
    
}

void reverse_16(u_int16_t *p) {
    u_int8_t *x = (u_int8_t *)p;
    u_int16_t y = *p;

    x[1] = y & 0xFF;
    x[0] = (y >> 8) & 0xFF;
}

void reverse_32(u_int32_t *p) {
    u_int8_t *x = (u_int8_t *) p;
    u_int32_t y = *(u_int32_t *) x;
    x[3] = y & 0xFF;
    x[2] = (y >> 8) & 0xFF;
    x[1] = (y >> 16) & 0xFF;
    x[0] = (y >> 24) & 0xFF;
}


// 将块中存储的小端序数据转换为大端序
void reverse_block(struct Block *block) {
    u_int32_t *p;
    struct BPB *bpb;
    DIREnt *dirEnt, *tempEnt;
    LongNameEnt *longEnt;

    switch (block->type)
    {
    case BLOCK_FILE:
    case BLOCK_FREE:
        break;

    case BLOCK_BPB:
        bpb = (struct BPB *)block->data;
        reverse_16(&bpb->BPB_BytesPerSec);
        reverse_16(&bpb->BPB_RsvdSecCnt);
        reverse_16(&bpb->BPB_RootEntCnt);
        reverse_16(&bpb->BPB_TotSec16);
        reverse_16(&bpb->BPB_FATSz16);
        reverse_16(&bpb->BPB_SecPerTrk);
        reverse_16(&bpb->BPB_NumHeads);

        reverse_32(&bpb->BPB_HiddSec);
        reverse_32(&bpb->BPB_TotSec32);

        reverse_32(&bpb->BPB_FATSz32);
        reverse_16(&bpb->BPB_ExtFlags);
        reverse_16(&bpb->BPB_FSVer);
        reverse_32(&bpb->BPB_RootClus);
        reverse_16(&bpb->BPB_FSInfo);
        reverse_16(&bpb->BPB_BkBootSec);
        reverse_32(&bpb->BS_VolID);

        // 翻转FSInfo
        reverse_32(&fsinfo->FSI_LeadSig);
        reverse_32(&fsinfo->FSI_StrucSig);
        reverse_32(&fsinfo->FSI_Free_Count);
        reverse_32(&fsinfo->FSI_Nxt_Free);
        reverse_32(&fsinfo->FSI_TrailSig);
        break;

    case BLOCK_DIR:
        // 分两种情况: 目录项和长目录项
        dirEnt = (DIREnt *)block->data;
        for (int i = 0; i < BY2BLK / BY2DIRENT; i++) {
            if (dirEnt[i].DIR_Attr == ATTR_LONG_NAME_MASK) {
                // 长文件名项
                longEnt = (LongNameEnt *)(dirEnt+i);
                reverse_16(&longEnt->LDIR_FstCLusLO);
            }
            else {
                tempEnt = dirEnt+i;
                reverse_16(&tempEnt->DIR_CrtTime);
                reverse_16(&tempEnt->DIR_CrtDate);
                reverse_16(&tempEnt->DIR_LstAccDate);
                reverse_16(&tempEnt->DIR_FstClusHI);
                reverse_16(&tempEnt->DIR_WrtTime);
                reverse_16(&tempEnt->DIR_WrtDate);
                reverse_16(&tempEnt->DIR_DstClusLO);
                reverse_32(&tempEnt->DIR_FileSize);
            }
        }
        break;

    case BLOCK_FAT:
        p = (u_int32_t *)block->data;
        for (int i = 0; i < BY2BLK/4; i++) {
            reverse_32(p+i);
        }
        break;
    
    default:
        break;
    }
}

void finish_fs(char *path) {
    int fd;

    // 更新FSInfo
    fsinfo->FSI_Nxt_Free = next_cluster;
    fsinfo->FSI_Free_Count = NDATABLOCK - (next_cluster - 2);

    fd = open(path, O_RDWR | O_CREAT, 0666);
    int numWrite = 200;
    for (int i = 0; i < numWrite; i++) { // 应当写入NBLOCK块，但为了节省时间和减少磁盘写入，写入200块即可
        if(isReverse) reverse_block(blocks+i);
        write(fd, blocks[i].data, BY2BLK);
    }

    close(fd);
}

int main(int argc, char **argv) {
    int i, argpos;

    init_disk();

    if (argc < 2) goto showHelp; // 参数只有一项，报错
    else if (argc >= 2 && strcmp(argv[1], "-h") == 0) 
        goto showHelp; // 帮助选项

    assert(argc >= 2);
    if(strcmp(argv[1], "-l") == 0) {
        argpos = 2;
        isReverse = 0;
    } else {
        argpos = 1;
    }

    if (argpos+2 > argc)
        goto showHelp; // 参数不足，没有fs.img项目和[-r/file]选项

    if(strcmp(argv[argpos+1], "-r") == 0) {
        if (argpos+3 > argc)
            goto showHelp; // 参数不足，-r后面没有目录项
        for (i = argpos+2; i < argc; ++i) {
            write_directory(rootDirEnt, argv[i]);
        }
    }
    else {
        for(i = argpos+1; i < argc; ++i) {
            write_file(rootDirEnt, argv[i]);
        }
    }

    finish_fs(argv[argpos]);
    return 0;

showHelp:
    fprintf(stderr, "\
Usage: fsformat [-l] gxemul/fs.img files...\n\
       fsformat [-l] gxemul/fs.img -r DIR\n\
       fsformat -h\n\
       -l: little endian; or else big endian.\n");
    exit(0);
}