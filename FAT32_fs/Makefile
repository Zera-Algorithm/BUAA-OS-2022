INCLUDES = -I../include/ -I../user/

all: fsformat fs.o

fsformat: fsformat.c fat32.h
	gcc fsformat.c -o fsformat
	chmod +x fsformat

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm fsformat

include ../include.mk