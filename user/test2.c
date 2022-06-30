#include "lib.h"

char buf[81920];
void umain() {
    int fdnum;
    struct Stat state;
    writef("Hello!\n");
    // 1. 读取不存在的文件系统
    fdnum = open("/root2/testfile", O_RDWR);
    writef("fdnum1 = %d\n", fdnum);
    user_assert(fdnum < 0);

    // 2. 读取不存在的文件
    fdnum = open("/root0/not-found", O_RDWR);
    writef("fdnum2 = %d\n", fdnum);
    user_assert(fdnum == -E_NOT_FOUND);

    fdnum = open("/root1/not-found", O_RDWR);
    writef("fdnum3 = %d\n", fdnum);
    user_assert(fdnum == -E_NOT_FOUND);    

    return;
}