#include "lib.h"

char buf[60];

// 输入字符，以回车为结束
// 同时负责回显字符
void scanf(char *buffer) {
    int i = 0;
    while (1) {
        char c = syscall_cgetc();
        if (c == '\r') {
            // 表示回车
            buffer[i] = '\0';
            writef("\n", c); // 输出'\r'是回到行的起始位置重新开始写
            return;
        }
        else if (c == 127) { // 退格键
            // 如果输入的内容全部删除完毕之后就不能继续删除了
            if (i != 0) {
                i -= 1;
                writef("%c %c", 8, 8); // 向后退一格
            }
        }
        else {
            buffer[i++] = c;
            writef("%c", c);
        }
    }
}

void lsdir(char *str) {
    int fd, r;
    struct Stat state;
    r = stat(str, &state);
    DIREnt dirent;
    if (r < 0) {
        writef("stat error: %d\n", r);
    }
    else if (!state.st_isdir) {
        writef("%s ", state.st_name);
        writef("is not a directory!\n");
    }
    else {
        writef("Before open: dir's size = %d\n", state.st_size);
        fd = open(str, O_RDONLY);
        writef("End open.\n");
        
        while(1) {
            r = readn(fd, &dirent, sizeof(DIREnt));
            if (r == 0) {
                writef("\n");
                close(fd);
                return;
            }
            else {
                if (dirent.DIR_Name[0]) {
                    writef("%s ", dirent.DIR_Name);
                }
            }
        }
    }
}

void umain() {
    // 接受键盘输入
    int i;
    // 等待文件系统进程初始化完毕
    for (i = 0; i < 20; i++) {
        syscall_yield();
    }

    lsdir("/root1/");
    // while(1) {
    //     writef("ls: ");
    //     scanf(buf);
    //     lsdir(buf);
    // }
}