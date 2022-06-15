#include "lib.h"

void umain() {
    int fdnum;
    fdnum = open("/testfile", O_RDWR);
    if (fdnum < 0) user_panic("open error!");
    write(fdnum, "I changed the content of newmotd!", 35);
    close(fdnum);
    return 0;
}