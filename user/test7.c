#include "lib.h"

char buf[600];

int lsdir(char *str) {
    int fd, r, pos;
    struct Stat state;
    r = stat(str, &state);
    DIREnt dirent;
    if (r < 0) {
        return r;
    }
    else if (!state.st_isdir) {
        return -E_INVAL;
    }
    else {
        fd = open(str, O_RDONLY);

        buf[0] = 0;
        pos = 0;
        while(1) {
            r = readn(fd, &dirent, sizeof(DIREnt));
            if (r == 0) {
                buf[pos] = 0;
                close(fd);
                return;
            }
            else if (r < 0) {
                writef("r = %d\n", r);
            }
            else {
                if (dirent.DIR_Name[0]) {
                    strcpy(buf + pos, dirent.DIR_Name);
                    pos += strlen(dirent.DIR_Name);
                    buf[pos++] = ' ';
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

// ----------------root-----------------
    user_assert(lsdir("/root0") >= 0);
    writef(buf);
    user_assert(strcmp(buf, "motd newmotd testfile ") == 0);

    user_assert(lsdir("/root0/") >= 0);
    user_assert(strcmp(buf, "motd newmotd testfile ") == 0);

    user_assert(lsdir("/root1") >= 0);
    user_assert(strcmp(buf, "test ") == 0);

    user_assert(lsdir("/root1/") >= 0);
    user_assert(strcmp(buf, "test ") == 0);

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

    user_assert(lsdir("/root1/test/bin/././././main/..") >= 0); writef(buf);
    user_assert(strcmp(buf, ". .. main test hello.c dev ") == 0);

    user_assert(lsdir("/root1/test/etc") >= 0); writef(buf);
    user_assert(strcmp(buf, ". .. conf.json ") == 0);
}