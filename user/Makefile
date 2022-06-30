INCLUDES = -I../include/

#CROSS_COMPILE := /usr/eldk/usr/bin/mips_4KC-
#CC			  := $(CROSS_COMPILE)gcc
#CFLAGS		  := -O -G 0 -mno-abicalls -fno-builtin -Wa,-xgot -Wall -fPIC
#LD			  := $(CROSS_COMPILE)ld
# For embedding one program in another

USERLIB := printf.o \
		print.o \
		libos.o \
		fork.o \
		pgfault.o \
		syscall_lib.o \
		ipc.o \
		string.o \
		fd.o \
		pageref.o \
		file.o \
		pipe.o \
		fsipc.o \
		console.o \
		fprintf.o

CFLAGS += -nostdlib -static

all: devtst.x devtst.b tltest.x tltest.b \
	fstestread.x fstestread.b fstestwrite.x fstestwrite.b \
	fktest.x fktest.b pingpong.x pingpong.b idle.x fstest.x fstest.b $(USERLIB) entry.o syscall_wrap.o \
	test1.x test1.b test2.x test2.b test3.x test3.b \
	# test4.x test4.b test5.x test5.b \
	# test6.x test6.b test7.x test7.b test8.x test8.b test9.x test9.b test10.x test10.b

%.x: %.b.c
	echo cc1 $<
	$(CC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	echo create $@
	echo bintoc $* $< > $@~
	chmod +x ./bintoc
	./bintoc $* $< > $@~ && mv -f $@~ $@
#	grep \. $@

%.b: entry.o syscall_wrap.o %.o $(USERLIB)
	echo ld $@
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./user.lds $^

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	echo as $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: lib.h

.PHONY: clean

clean:
	rm -rf *~ *.o *.b.c *.x *.b

include ../include.mk
