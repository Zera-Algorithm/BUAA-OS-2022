#include "lib.h"
// 整体测试

void readfullfile(const char *path, char *buffer) {
    struct Stat state;
    stat(path, &state);
    writef("The length of the file is %d.\n", state.st_size);
    int fdnum = open(path, O_RDWR);
    if (fdnum < 0) {
        user_panic("Read Error! %d", fdnum);
    }
    readn(fdnum, buffer, state.st_size);
    close(fdnum);
}

void longPrint(char *buffer) {
    int i = 0;
    for (i = 0; buffer[i]; i++) {
        writef("%c", buffer[i]);
    }
}

char buf[81920];
void umain() {
    int fdnum, len;
    struct Stat state;

    // 1. 读取文件内容
    readfullfile("/root1/lfile", buf);

    // 2. 获取文件长度
    len = strlen(buf);

    // 3. 输出内容
    writef("The content of the file(/root1/lfile): ");
    longPrint(buf);

    // 4. 检查文件首尾是否正常
    user_assert(strcmp("This is the end of the file!\nHappy this day!\n", buf+len-45) == 0);
    buf[97] = 0;
    user_assert(strcmp("This is a file to test function.\nIt's size must be longer than 4KB.\nThe following is the content:", buf) == 0);
    
    writef("long file test passed!\n");
    return 0;
}