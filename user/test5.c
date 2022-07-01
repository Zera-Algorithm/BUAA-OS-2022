#include "lib.h"
// 测试文件删除

void umain() {
    // ---------------------/root0/motd
    int fd;
    fd = open("/root0/motd", O_RDWR);
    user_assert(fd >= 0); // fd可以为0
    close(fd);

    remove("/root0/motd");
    user_assert(open("/root0/motd", O_RDWR) == -E_NOT_FOUND);

    // --------------------root1
    fd = open("/root1/motd", O_RDWR);
    user_assert(fd >= 0);
    close(fd);

    remove("/root1/motd");
    user_assert(open("/root1/motd", O_RDWR) == -E_NOT_FOUND);

    writef("file remove passed!");
}