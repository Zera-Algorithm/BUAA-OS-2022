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
