#include "lib.h"
// 读、写、删除
// 创建：create + read
// 长文件名文件
char buf[512];

void umain() {
    int fd, r, i;
    
    writef("task 1\n");
    fd = open("/root1/test/bin/abcdefghijklmnopqrstuvwxyz", O_RDONLY);
    user_assert(fd >= 0);
    readn(fd, buf, 512);
    writef(buf);
    user_assert(strcmp(buf, "/root1/test/bin") == 0);
    close(fd);

    writef("task 2\n");
    fd = open("/root1/test/bin/I_am_a_long_name_that_contains_date_of_the_file_s_creation_and_the_creator_s_name_which_is_zrp_2022_7_2_that_is_the_end", O_RDONLY);
    readn(fd, buf, 512);
    user_assert(strcmp(buf, "I_am_a_long_name_that_contains_date_of_the_file_s_creation_and_the_creator_s_name_which_is_zrp_2022_7_2_that_is_the_end") == 0);
    close(fd);

    writef("task 3\n");
    fd = open("/root1/test/bin/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccccc_end", O_RDWR);
    write(fd, "This is me!", 12);
    seek(fd, 0);
    readn(fd, buf, 12);
    user_assert(strcmp(buf, "This is me!") == 0);
    close(fd);

    writef("task 4\n");
    user_assert(open("/root1/test/bin/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa123bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccc_end", O_RDONLY) >= 0);
    remove("/root1/test/bin/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa123bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccc_end");
    user_assert(open("/root1/test/bin/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa123bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccc_end", O_RDONLY) < 0);

    writef("task 5\n");
    fd = open("/root1/test/bin/looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong_name", O_CREAT | O_RDWR);
    user_assert(fd >= 0);
    write(fd, "This is a long file!!!!!!!!!!!!!!!!!!!", 39);
    close(fd);

    writef("task 6\n");
    fd = open("/root1/test/bin/looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong_name", O_CREAT | O_RDWR);
    user_assert(fd >= 0);
    readn(fd, buf, 50);
    user_assert(strcmp("This is a long file!!!!!!!!!!!!!!!!!!!", buf) == 0);
    close(fd);
}