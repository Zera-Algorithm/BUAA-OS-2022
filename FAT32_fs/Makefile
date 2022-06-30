INCLUDES = -I../include/ -I../user/

all: fsformat fs.o

fsformat: fsformat.c fat32.h
	gcc fsformat.c $(INCLUDES) -o fsformat
	chmod +x fsformat

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm fsformat
	rm *.o

include ../include.mk