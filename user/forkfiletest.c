#include "lib.h"

char buf[100];
char childbuf[100];
void umain() {
    int fdnum, i, r;
    fdnum = open("data.txt", O_RDONLY);
    if (fdnum < 0) user_panic("failed to Open file!");
    
    i = read(fdnum, buf, 5);
    buf[i] = 0;
    
    writef("[father] Data.txt's content is: %s\n", buf);
    writef("[father] Now fd offset = %d.\n", ((struct Fd *)num2fd(fdnum))->fd_offset);
    
    r = fork();
    if (r < 0) user_panic("fail to fork.");
    if (r == 0) {
        writef("[Child] Now fd offset = %d.\n", ((struct Fd *)num2fd(fdnum))->fd_offset);
        
        i = read(fdnum, childbuf, 40);
        childbuf[i] = 0;

        writef("[Child] The rest of data.txt is: %s", childbuf);
    }
}