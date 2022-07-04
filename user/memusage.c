#include "lib.h"

void umain() {
    int i, r, used = 0;
    int npages = 64 * 1024 * 1024 / 4096;
    fwritef(1, "Total page count = %d\n", npages);
    for (i = 0; i < npages; i++) {
        if (pages[i].pp_ref != 0) {
            used += 1;
        }
    }
    int kb = used * 4;
    fwritef(1, "Used Mem: %d KB.\n", kb);
}