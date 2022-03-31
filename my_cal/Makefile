# Main makefile
#
# Copyright (C) 2007 Beihang University
# Written by Zhu Like ( zlike@cse.buaa.edu.cn )
#

# ========= Configuration =========

INCLUDES      := -I../include/

# ======== End of configuration ===

all: my_driver.o my_cal.o

my_driver.o: my_driver.S
	$(CC) $(CFLAGS) $(INCLUDES) -c my_driver.S

my_cal.o: my_cal.c
	$(CC) $(DEFS) $(CFLAGS) $(INCLUDES) -c my_cal.c

.PHONY: clean

clean:
	rm -rf *.o

include ../include.mk
