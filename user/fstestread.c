#include "lib.h"

char buf[100];
void umain() {
    int fdnum;
    fdnum = open("/testfile", O_RDWR);
    if (fdnum < 0) user_panic("open error!");
    readn(fdnum, buf, 35);
    writef("The content of the file: %s", buf);
    close(fdnum);
    return 0;
}