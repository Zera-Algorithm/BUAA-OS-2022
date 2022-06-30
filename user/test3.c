#include "lib.h"
// 整体测试

void readfullfile(const char *path, char *buffer) {
    struct Stat state;
    int fdnum = open(path, O_RDWR);
    if (fdnum < 0) {
        user_panic("Read Error! %d", fdnum);
    }
    stat(path, &state);
    readn(fdnum, buffer, state.st_size);
    close(fdnum);
}

char buf[81920];
void umain() {
    int fdnum;
    struct Stat state;

    // 1. 读取文件内容
    readfullfile("/root1/lfile", buf);

    // 3. 输出内容
    writef("The content of the file(/root1/lfile): %s", buf);
    return 0;
}