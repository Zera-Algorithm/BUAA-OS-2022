#include "test11.h"

char buf[40960];
void umain() {
    int fd, i, r;

    fd = open("/root1/test/bin/new_long_file_c", O_RDWR);
    readn(fd, buf, sizeof(msg));
    user_assert(fd >= 0);
    user_assert(strcmp(buf, msg) == 0);
    close(fd);

    fd = open("/root1/motd", O_RDWR);
    user_assert(fd >= 0);
    readn(fd, buf, 20);
    user_assert(strcmp(buf, "MONDAYWorld!\nI'm Zrp") == 0);
    close(fd);

    fd = open("/root1/lfile", O_RDONLY);
    user_assert(fd == -E_NOT_FOUND);

    fd = open("/root1/longFile", O_RDWR);
    for (i = 0; i < nlongmsg; i++) {
        writef("#line %d\n", i);
        readn(fd, buf, strlen(long_msg[i]));
        user_assert(strcmp(long_msg[i], buf) == 0);
    }
    close(fd);

    writef("[note] write back test passed!\n");
}