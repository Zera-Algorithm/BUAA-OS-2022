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

// 在字符串前面插入字符串
void strins(char *buf, char *str, int len) {
	int lbuf = strlen(buf);
	int i;
	for (i = lbuf; i >= 0; i--) {
		buf[i+len] = buf[i];
	}
	for (i = 0; i < len; i++) {
		buf[i] = str[i];
	}
}

void lsdir(char *str) {
    int fd, r;
    struct Stat state;
    r = stat(str, &state);
    
    char name[MAXNAMELEN];
    name[0] = 0;
    
    DIREnt dirent;
    LongNameEnt *longEnt;
    struct File file;

    if (r < 0) {
        writef("stat error: %d\n", r);
    }
    else if (!state.st_isdir) {
        writef("%s ", state.st_name);
        writef("is not a directory!\n");
    }
    else {
        fd = open(str, O_RDONLY);
        
        char t = str[6];
        str[6] = 0;
        if (strcmp(str, "/root1") == 0) {
            // FATFS类型的目录项
            while(1) {
                r = readn(fd, &dirent, sizeof(DIREnt));
                if (r == 0) {
                    writef("\n");
                    close(fd);
                    break;
                }
                else {
                    if (dirent.DIR_Name[0] != 0 && dirent.DIR_Name[0] != 0xE5) {
                        if (dirent.DIR_Attr == ATTR_LONG_NAME_MASK) {
                            longEnt = (LongNameEnt *)(&dirent);
                            // 是第一项
                            if (longEnt->LDIR_Ord & LAST_LONG_ENTRY) {
                                name[0] = 0;
                            }

                            strins(name, longEnt->LDIR_Name3, 4);
                            strins(name, longEnt->LDIR_Name2, 12);
                            strins(name, longEnt->LDIR_Name1, 10);
                        }
                        else {
                            strins(name, dirent.DIR_Name, 11);
                            // 找到一个目录项
                            writef("%s ", name);
                            name[0] = 0;
                        }
                    }
                }
            }
        }
        else {
            while (1) {
                r = readn(fd, &file, sizeof(file));
                if (r == 0) {
                    writef("\n");
                    close(fd);
                    break;
                }
                else {
                    if (file.f_name[0] == 0) continue;
                    writef("%s ", file.f_name);
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

    // lsdir("/root1/");
    while(1) {
        writef("ls: ");
        scanf(buf);
        lsdir(buf);
    }
}