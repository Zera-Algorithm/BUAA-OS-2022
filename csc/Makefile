all:
	cd code && gcc -c -I../include/ main.c && gcc -c -I../include/ fibo.c
	gcc code/fibo.o code/main.o -o fibo

clean:
	rm code/fibo.o code/main.o

