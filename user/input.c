#include "lib.h"

/*
 * writef输出的控制字符列表
 * \r: 回到行首
 * \n: 到下一行行首（换行）
 * 8: 退格（删除一个字符）
 * 
 * gxemul读入的字符对应关系：
 * Enter键：\r
 * 退格键: 127
 * 
 */

void umain() {
    // 接受键盘输入
    int i;
    // 等待文件系统进程初始化完毕
    for (i = 0; i < 10; i++) {
        syscall_yield();
    }
    writef("12345678%c90\n", 8);
    while (1) {
        char c = syscall_cgetc();
        // writef("%d ", c);
        writef("%c", c);
    }
}