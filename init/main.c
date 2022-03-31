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
	printf("%0----0-04dhello.\n", (int)23);
}

int main()
{
	printf("main.c:\tmain is start ...\n");

	/* This code is only for printf testing! */
	 test();
	/* This is the end */

	mips_init();
	panic("main is over is error!");

	return 0;
}
