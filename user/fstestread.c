#include "lib.h"

char buf[100];
void umain() {
    int fdnum;
    struct Stat state;

    // 1. 读取文件内容
    fdnum = open("/root0/testfile", O_RDWR);
    if (fdnum < 0) user_panic("open error!");

    // 2. 读取文件信息
    stat("/root0/testfile", &state);
    readn(fdnum, buf, state.st_size);

    // 3. 输出内容
    writef("The content of the file: %s", buf);
    close(fdnum);

    /*-------------FATFS测试----------------*/
    fdnum = open("/root1/motd", O_RDONLY);
    if (fdnum < 0) user_panic("open error! code = %d", fdnum);

    readn(fdnum, buf, 40);
    writef("The content of /root1/motd is: %s", buf);

    close(fdnum);
    return 0;
}