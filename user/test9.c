#include "lib.h"

char newmotd_msg[] = "This is the NEW message of the day!";
char buf[4096];

void longPrint(char *buffer) {
    int i = 0;
    for (i = 0; buffer[i]; i++) {
        writef("%c", buffer[i]);
    }
}

void umain() {
    int pid, r, fd, i;
    pid = fork();
    if (pid < 0) {
        writef("fork error!\n");
    }
    else if (pid == 0) {
        for (i = 0; i < 40; i++) {
            fd = open("/root0/newmotd", O_RDWR);
            user_assert(fd >= 0);
            readn(fd, buf, sizeof(newmotd_msg)-1);
            user_assert(strcmp(buf, newmotd_msg) == 0);
            close(fd);
        }
        fd = open("/root1/test/home/MyPic.pic", O_RDONLY);
        readn(fd, buf, 4096);

        writef("/root1/test/home/MyPic.pic:");
        longPrint(buf);
        writef("\n");
        close(fd);
    }
    else {
        for (i = 0; i < 40; i++) {
            fd = open("/root0/newmotd", O_RDWR);
            user_assert(fd >= 0);
            readn(fd, buf, sizeof(newmotd_msg)-1);
            user_assert(strcmp(buf, newmotd_msg) == 0);
            close(fd);
        }
        fd = open("/root1/test/home/pic/python.pic", O_RDONLY);
        readn(fd, buf, 4096);
        writef("/root1/test/home/pic/python.pic:");
        longPrint(buf);
        writef("\n");
        close(fd);
    }
}