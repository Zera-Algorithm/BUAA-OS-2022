#include "lib.h"
// 测试写入超出原来的容量，文件扩增

char longmsg[] = "This is a long msg, which is designed to test the functionality of the FS writing. Now Let's see!";
char buf[200];
void umain() {
    int fd;
    fd = open("/root1/motd", O_RDWR);
    write(fd, longmsg, sizeof(longmsg));
    seek(fd, 0); // 回归到文件开始位置
    readn(fd, buf, 200);
    user_assert(strcmp(longmsg, buf) == 0);
    writef("write test passed!");
}