#include "test11.h"

void umain() {
    int fd, i, r;

    writef("########## test create #########\n");
    fd = open("/root1/test/bin/new_long_file_c", O_CREAT | O_RDWR);
    user_assert(fd >= 0);
    write(fd, msg, sizeof(msg));
    close(fd);

    writef("########## test write #########\n");
    fd = open("/root1/motd", O_RDWR);
    user_assert(fd >= 0);
    write(fd, "MONDAY", 6);
    close(fd);

    writef("########## test remove #########\n");
    r = remove("/root1/lfile");
    user_assert(r >= 0);

    writef("########## test long write #########\n");
    fd = open("/root1/longFile", O_RDWR | O_CREAT);
    for (i = 0; i < nlongmsg; i++) {
        write(fd, long_msg[i], strlen(long_msg[i]));
    }
    close(fd);

    writef("########## test sync #########\n");
    // 将所有变更写入磁盘
    fsipc_sync();
    writef("All changes are sync into disk!\n");
}