INCLUDES = -I../include/ -I../user/

FSIMGFILES := motd lfile fat32.h

all: fs1.img fs.o

fs1.img: fsformat.c fat32.h
	gcc fsformat.c $(INCLUDES) -o fsformat
	chmod +x fsformat
	./fsformat ../gxemul/fs1.img $(FSIMGFILES)

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm fsformat
	rm *.o

include ../include.mk