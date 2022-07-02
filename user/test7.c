#include "lib.h"

char buf[600];

// 在字符串前面插入字符串
void strins(char *_buf, char *str, int len) {
	int lbuf = strlen(_buf);
	int i;
	for (i = lbuf; i >= 0; i--) {
		_buf[i+len] = _buf[i];
	}
	for (i = 0; i < len; i++) {
		_buf[i] = str[i];
	}
}

int lsdir(char *str) {
    int fd, r, pos = 0;
    struct Stat state;
    r = stat(str, &state);
    
    char name[MAXNAMELEN];
    name[0] = 0;
    
    DIREnt dirent;
    LongNameEnt *longEnt;
    struct File file;

    if (r < 0) {
        return r;
    }
    else if (!state.st_isdir) {
        return -E_INVAL;
    }
    else {
        buf[pos] = 0;
        fd = open(str, O_RDONLY);
        
        char t = str[6];
        str[6] = 0;
        if (strcmp(str, "/root1") == 0) {
            // FATFS类型的目录项
            while(1) {
                r = readn(fd, &dirent, sizeof(DIREnt));
                if (r == 0) {
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
                            strcpy(buf+pos, name);
                            pos += strlen(name);
                            buf[pos++] = ' ';
                            // writef("root1:%s \n", name);
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
                    close(fd);
                    break;
                }
                else {
                    if (file.f_name[0] == 0) continue;
                    strcpy(buf+pos, file.f_name);
                    pos += strlen(file.f_name);
                    buf[pos++] = ' ';
                    // writef("%s ", file.f_name);
                }
            }
        }
        buf[pos] = 0;
        return 0;
    }
}

void umain() {
    // 接受键盘输入
    int i;
    // 等待文件系统进程初始化完毕
    for (i = 0; i < 20; i++) {
        syscall_yield();
    }

// ----------------root-----------------
    user_assert(lsdir("/root0") >= 0);
    // writef(buf);
    user_assert(strcmp(buf, "motd newmotd testfile test_fsformat.sh ") == 0);

    user_assert(lsdir("/root0/") >= 0);
    // writef(buf);
    user_assert(strcmp(buf, "motd newmotd testfile test_fsformat.sh ") == 0);

    user_assert(lsdir("/root1") >= 0);
    // writef(buf);
    user_assert(strcmp(buf, "motd lfile test ") == 0);

    user_assert(lsdir("/root1/") >= 0);
    user_assert(strcmp(buf, "motd lfile test ") == 0);

// -------------------- . / .. ----------------
    user_assert(lsdir("/root0/.") < 0);
    user_assert(lsdir("/root1/not-found") < 0);
    user_assert(lsdir("/root1/./.") < 0);

    user_assert(lsdir("/root1/test/././.") >= 0);
    writef(buf);
    user_assert(strcmp(buf, ". .. lib home etc bin src ") == 0);
    
    user_assert(lsdir("/root1/test/../test") >= 0);
    user_assert(strcmp(buf, ". .. lib home etc bin src ") == 0);

    user_assert(lsdir("/root1/test/bin/main/../../bin/main/./././../main") >= 0);
    user_assert(strcmp(buf, ". .. file2.txt file1.txt ") == 0);

    user_assert(lsdir("/root1/test/etc") >= 0); writef(buf);
    user_assert(strcmp(buf, ". .. conf.json ") == 0);
}