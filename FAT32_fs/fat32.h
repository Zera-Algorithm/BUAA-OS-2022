// 定义类型
typedef unsigned char            u_int8_t;
typedef short                     int16_t;
typedef unsigned short          u_int16_t;
typedef int                       int32_t;
typedef unsigned int            u_int32_t;
typedef unsigned                     uint;

#pragma pack(1)
// 取消C编译器对结构体的自动对齐，保持结构体的偏移和占用空间
// 符合要求

struct BPB {
    // FAT12/16/32共享区域
    char BS_jmpBoot[3];
    char BS_OEMName[8];
    u_int16_t BPB_BytesPerSec; // 512
    u_int8_t BPB_SecPerClus; // 8
    u_int16_t BPB_RsvdSecCnt; // 8
    u_int8_t BPB_NumFATs; // 1
    u_int16_t BPB_RootEntCnt; // 0
    u_int16_t BPB_TotSec16; // 0
    u_int8_t BPB_Media; // 0xF8
    u_int16_t BPB_FATSz16; // 0
    u_int16_t BPB_SecPerTrk; // 0
    u_int16_t BPB_NumHeads; // 0
    u_int32_t BPB_HiddSec; // 0
    u_int32_t BPB_TotSec32; // 总扇区数

    // 32位独享
    u_int32_t BPB_FATSz32; // FAT的扇区数
    u_int16_t BPB_ExtFlags; // 128
    u_int16_t BPB_FSVer; // 0
    u_int32_t BPB_RootClus; // Root的簇号，设为2
    u_int16_t BPB_FSInfo; // 1
    u_int16_t BPB_BkBootSec; // 0
    u_int8_t BPB_Reserved[12]; // 0
    u_int8_t BS_DrvNum; // 0
    u_int8_t BS_Reserved1; // 0
    u_int8_t BS_BootSig; // 0x29
    u_int32_t BS_VolID; // 卷ID，与下面的卷标对应
    char BS_VolLab[11]; // 卷标
    char BS_FilSysType[8]; // "FAT32 "，只有字面的意义
    char padding[420]; // 启动代码区
    char Signature_word[2]; // 0x55(510), 0xAA(511)
};

struct FSInfo {
    u_int32_t       FSI_LeadSig; // magic num
    u_int8_t        FSI_Reserved1[480];
    u_int32_t       FSI_StrucSig; // magic num
    u_int32_t       FSI_Free_Count; // 可用的簇数
    u_int32_t       FSI_Nxt_Free; // 下一个空间的簇
    u_int8_t        FSI_Reserved2[12];
    u_int32_t       FSI_TrailSig; // magic num
}; // 文件系统信息结构体，挂载在1号扇区

/*
 * 如何决定使用的FAT文件系统类型？
 * 这由磁盘的簇数唯一确定：
 * N < 4085: FAT12
 * 4086 <= N < 65525: FAT16
 * N >= 65525: FAT32
 * 以簇大小为4K，共有65536个簇计算，所需要的磁盘大小为256m。
 */

// 一些写死的磁盘空间参数
#define NBLOCK      65601
// 写死的块数（每块4K）
#define NFATBLOCK   64

// FAT表占用8块
#define NBPBBLOCK   1
// BPB部分占用1块
#define NDATABLOCK  (NBLOCK-NFATBLOCK-NBPBBLOCK)

#define BY2BLK      4096
#define BY2CLUS     BY2BLK
#define ROOTCLUS    2

#define CLUS2BLK(x) (NBPBBLOCK + NFATBLOCK + (x) - 2)
#define BLK2CLUS(x) (x - NBPBBLOCK - NFATBLOCK + 2)

#define BY2DIRENT   32
#define LAST_LONG_ENTRY     0x40

typedef struct DIREnt {
    char        DIR_Name[11]; // 第一位为0表示该目录项空闲
    u_int8_t    DIR_Attr; // 文件属性
    u_int8_t    DIR_NTRes; // 0
    u_int8_t    DIR_CrtTimeTenth; // 创建时间，暂不设置
    u_int16_t   DIR_CrtTime; // 创建时间，暂不设置
    u_int16_t   DIR_CrtDate; // 创建时间，暂不设置
    u_int16_t   DIR_LstAccDate; // 上次访问日期，暂不设置
    u_int16_t   DIR_FstClusHI; // 簇号的高位
    u_int16_t   DIR_WrtTime; // 上次写入时间，暂不设置
    u_int16_t   DIR_WrtDate; // 上次写入时间，暂不设置
    u_int16_t   DIR_DstClusLO; // 簇号的低位
    u_int32_t   DIR_FileSize; // 文件大小
} DIREnt; // 共32位

typedef struct LongNameEnt {
    char        LDIR_Ord; // 最后一项必须加一个LAST_LONG_ENTRY的mask
    char        LDIR_Name1[10];
    u_int8_t    LDIR_Attr; // 必须是ATTR_LONG_NAME_MASK(唯一判断的字段)
    u_int8_t    LDIR_Type; // 0
    u_int8_t    LDIR_Chksum; // 原文件名的校验和
    char        LDIR_Name2[12];
    u_int16_t   LDIR_FstCLusLO; // 0
    char        LDIR_Name3[4];
} LongNameEnt;

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)

#define MAXNAMELEN      1024
#define CHAR2LONGENT    26

enum CLUS_TYPE {
    CLUS_FREE    = 0,
    CLUS_BAD     = 0x0ffffff7,
    CLUS_FILEEND = 0xffffffff,
};