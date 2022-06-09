#include "lib.h"


void umain()
{
    int r, fdnum, n, i;
    char buf[200];
    fdnum = open("/newmotd", O_RDWR | O_ALONE);
    if ((r = fork()) == 0) {
        write(fdnum, "Good", 4);
        seek(fdnum, 0);
        n = read(fdnum, buf, 5); // 读出Good 
	    writef("[child] buffer is \'%s\'\n", buf);
    } else {
        for (i = 0; i < 20; i++)
            syscall_yield();
	    n = read(fdnum, buf, 5); // 读出This
	    writef("[father] buffer is \'%s\'\n", buf);
    }
    while(1);
}

/* expected output:
==================================================================
[father] buffer is 'This '
[child] buffer is 'This '
==================================================================
*/
