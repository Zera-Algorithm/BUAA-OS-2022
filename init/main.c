/*
 * Copyright (C) 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <printf.h>
#include <pmap.h>

void test() {
	printf("Binary = %b, Decimal = %d, Octal = %O, Unsigned = %u\n", 14, 455, 233, (int)(-56));
	// illegal inputs
	printf("This is an illegal input: %l, %ll, %.w\n");
	printf("Sizeof(long) = %d\n", sizeof(long int));
	printf("long num = %ld\n", (long int)10000l);
	printf("ladleft = %-5dThis is a.\n", 23);
	printf("rightlad = %7dThis is a.\n", 23456);
	printf("overwidth = %2dThis is a.\n", 12345);
	printf("Char = %.2c.\n", 'w');
	printf("Simple negative num = %d\n", (int)(-342));
	printf("0leftpad = %06.4d\n", (int)(-12));
	printf("%0----0-0-4dhello.\n", (int)23);
}

struct my_struct1 {
	int size;
	char c;
	int array[3];
};

struct my_struct2 {
	int size;
	char c;
	int array[2];
};

void struct_test() {
	struct my_struct1 t1;
	struct my_struct2 t2;
	t1.size=3; t1.c='b'; t1.array[0]=0; t1.array[1]=1; t1.array[2]=2;
	t2.size=2; t2.c='Q'; t2.array[0]=-1; t2.array[1]=-2;
	printf("%T_t1\n", &t1);
	printf("%T_t2\n", &t2);
	printf("%04T_t1\n", &t1);
	printf("%04T_t2\n", &t2);
	printf("%x, %x, %x.\n", &t1.size, &t1.c, t1.array);
}

int main()
{
	printf("main.c:\tmain is start ...\n");

	/* This code is only for printf testing! */
	// test();
	/* This is the end */
	struct_test();	

	mips_init();
	panic("main is over is error!");

	return 0;
}
