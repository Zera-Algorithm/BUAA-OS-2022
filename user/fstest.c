#include "lib.h"


void umain()
{
    int r, fdnum, fdnum2, fdnum3, n;
    char buf[200];
    fdnum = open("/newmotd", O_RDWR | O_ALONE);
    fdnum2 = open("/motd", O_RDWR | O_ALONE);
    fdnum3 = open("/newmotd", O_RDWR | O_ALONE);
    writef("[father] pageref = %d.\n", pageref(num2fd(fdnum)));
    if ((r = fork()) == 0) {
	    n = read(fdnum, buf, 10);
	    writef("[child] buffer is \'%s\'\n", buf);

        n = read(fdnum2, buf, 10);
	    writef("[child] buffer is \'%s\'\n", buf);

        n = read(fdnum3, buf, 10);
	    writef("[child] buffer is \'%s\'\n", buf);

        n = read(fdnum, buf, 10);
	    writef("[child] buffer is \'%s\'\n", buf);

        writef("[child] pageref = %d.\n", pageref(num2fd(fdnum)));

        n = read(fdnum, buf, 4);
        writef("[child] buffer is \'%s\'\n", buf);
    } else {
	    n = read(fdnum, buf, 10);
	    writef("[father] buffer is \'%s\'\n", buf);

        n = read(fdnum2, buf, 10);
	    writef("[father] buffer is \'%s\'\n", buf);

        n = read(fdnum3, buf, 10);
	    writef("[father] buffer is \'%s\'\n", buf);

        n = read(fdnum, buf, 10);
	    writef("[father] buffer is \'%s\'\n", buf);

        writef("[father] pageref = %d.\n", pageref(num2fd(fdnum)));
        
        strcpy(buf, "1234");
        write(fdnum, buf, 4);
    }
    while(1);
}

/* expected output:
==================================================================
[father] buffer is 'This '
[child] buffer is 'This '
==================================================================
*/
