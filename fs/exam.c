#include "fs.h"
#include "lib.h"
#include <mmu.h>

/* 创建数据 */
void init_data(int data[], int size) {
    int i;
    for (i = 0; i < size; i++) {
        data[i] = i + size;
    }
}

/* 打印数据 */
void print_data(int data[], const char* prefix, int size) {
    writef("%s", prefix);
    int i;
    for (i = 0; i < size; i++) {
        if (i != 0 && i % 128 == 0) writef("\n");
        writef("%d ", data[i]);
    }
    writef("\n");
}

void umain(void) {
    /* 检查 time read */
    int time = time_read();
    writef("current time is: %d\n", time);
    /* 检查 raid0 write & read */
    int data[6][128], read[6][128], i, j;
    init_data(data[0], 128*6);
    print_data(data[0], "Standard data is: \n", 128*6);

	raid0_write(24, data[0], 6);
	raid0_read(24, read[0], 6);
	/*for (i = 0; i < 128; i++) {
		for (j = 0; j < 6; j++) {
			writef("i = %d, j = %d, data = %d, read = %d\n", i, j, data[j][i], read[j][i]);
			if (data[j][i] != read[j][i]) {
				user_panic("Read error!");
			}
		}
	}*/
}
