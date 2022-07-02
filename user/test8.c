// 父子进程多路先后读写测试
// 包括同时请求MOSFS和FATFS
#include "lib.h"

char str[60], buf[60];
char msg[] = "I send a message of ?.";
void umain() {
    int pid, fd, i;
    u_int whom, val;

    pid = fork();
    if (pid < 0) {
        writef("failed to fork!\n");
    }
    else if (pid == 0) {
        for (i = 0; i < 10; i++) {
            val = ipc_recv(&whom, 0, 0);
            fd = open("/root0/motd", O_RDWR);
            strcpy(str, msg);
            str[20] = i + '0';
            readn(fd, buf, sizeof(msg)-1);
            if (strcmp(buf, str) == 0) {
                writef("[*] /root0 write&read test %d passed.\n", i);
            } else {
                writef("[*] /root0 write&read test %d failed.\n", i);
                user_panic("read msg: %s", buf);
            }
            close(fd);
            ipc_send(whom, 0, 0, 0); // 表示这边读文件完毕了
        }

        for (i = 0; i < 10; i++) {
            val = ipc_recv(&whom, 0, 0);
            fd = open("/root1/test/bin/hello.c", O_RDWR);
            strcpy(str, msg);
            str[20] = i + '0';
            int r = readn(fd, buf, sizeof(msg)-1);
            if (strcmp(buf, str) == 0) {
                writef("[*] /root1 write&read test %d passed.\n", i);
            } else {
                writef("[*] /root1 write&read test %d failed.\n", i);
            }
            close(fd);
            ipc_send(whom, 0, 0, 0); // 表示这边读文件完毕了
        }
    }
    else {
        for (i = 0; i < 10; i++) {
            fd = open("/root0/motd", O_RDWR);
            fwritef(fd, "I send a message of %d.", i); // 格式化写入文件
            close(fd);
            ipc_send(pid, i, 0, 0);
            ipc_recv(&whom, 0, 0);
        }

        for (i = 0; i < 10; i++) {
            fd = open("/root1/test/bin/hello.c", O_RDWR);
            fwritef(fd, "I send a message of %d.", i); // 格式化写入文件
            close(fd);
            ipc_send(pid, i, 0, 0);
            ipc_recv(&whom, 0, 0);
        }
    }
}