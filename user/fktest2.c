#include "lib.h"

void umain() {
	int id;
	writef("I'm father!!\n");
	if ((id = fork()) == 0) {
		writef("child!\n");
		while(1) writef("I'm child!! id = %d, &id = %x\n", id, &id);
	}
	else {
		writef("father!!\n");
		while(1) writef("I'm father!! id = %d, &id = %x\n", id, &id);
	}

}
