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

struct list {
	int data;
	LIST_ENTRY(list) entry;
};

// macro test passed.
void test() {
	// all element usage is through pointer
	LIST_HEAD(list_head, list) head = LIST_HEAD_INITIALIZER(head);
	struct list elm1; elm1.data = 1;
	printf("%d", (&elm1)->data);
	LIST_INSERT_TAIL(&head, &elm1, entry);
	struct list elm2; elm2.data = 3;
	LIST_INSERT_TAIL(&head, &elm2, entry);
	struct list elm3; elm3.data = 5;
	LIST_INSERT_TAIL(&head, &elm3, entry);
	struct list elm0; elm0.data = -1;
	LIST_INSERT_BEFORE(&elm1, &elm0, entry);
	struct list elm4; elm4.data = 2;
	LIST_INSERT_AFTER(&elm1, &elm4, entry);
	
	struct list *i;
	LIST_FOREACH(i, &head, entry) printf("%d, ", i->data);
}

int main()
{
	printf("main.c:\tmain is start ...\n");
	// test();

	mips_init();
	panic("main is over is error!");

	return 0;
}
