#include "lib.h"
// 创建文件测试

char buf[81920];
char msg[] = "This is a new file!!";
void umain() {
    int fdnum, i;
    struct Stat state;

    // 1. open inexist file
    fdnum = open("/root0/newfile", O_RDWR);
    user_assert(fdnum < 0);

    // 2. create inexist file
    fdnum = open("/root0/newfile", O_CREAT | O_RDWR);
    write(fdnum, msg, sizeof(msg));
    close(fdnum);

    fdnum = open("/root0/newfile", O_CREAT | O_RDONLY);
    readn(fdnum, buf, 40);
    for (i = 0; msg[i]; i++) {
        if (buf[i] != msg[i]) {
            user_panic("Read content error: Expected %s, Actually %s\n", msg, buf);
        }
    }

    writef("root0 create passed!\n");


    //------------------root1----------------------------
    // 1. open inexist file
    fdnum = open("/root1/newfile", O_RDWR);
    user_assert(fdnum < 0);

    // 2. create inexist file
    fdnum = open("/root1/newfile", O_CREAT | O_RDWR);
    write(fdnum, msg, sizeof(msg));
    close(fdnum);

    fdnum = open("/root1/newfile", O_CREAT | O_RDONLY);
    readn(fdnum, buf, 40);
    for (i = 0; msg[i]; i++) {
        if (buf[i] != msg[i]) {
            user_panic("Read content error: Expected %s, Actually %s\n", msg, buf);
        }
    }

    writef("root1 create passed!\n");
}