// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include "lib.h"

void
umain(void)
{
	u_int who, i;
	char *str_send = (char *)(0x00600000);
	char *str_recv = (char *)(0x00800000);

	if ((who = fork()) != 0) {
		writef("I'm Father!\n");

		syscall_mem_alloc(syscall_getenvid(), (u_int)str_send, PTE_V | PTE_R);
		strcpy(str_send, "Hello World!\n");
		
		writef("Father send ipc.\n");
		ipc_send(who, 0, str_send, PTE_V | PTE_R);
		//user_panic("&&&&&&&&&&&&&&&&&&&&&&&&m");
		ipc_recv(&who, 0, 0);
		writef("Father: Modified str = %s\n", str_send);
	}
	else {
		writef("I'm child!\n");
		i = ipc_recv(&who, str_recv, 0);
		writef("Child recv Message %s, and value %d.\n", str_recv, i);
		writef("Child: Modifying message.\n");
		str_recv[0] = 'T';
		ipc_send(who, 0, 0, 0);
	}

	return;
}

