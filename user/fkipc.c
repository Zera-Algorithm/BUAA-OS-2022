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
		// get the ball rolling
		writef("\n@@@@@send 0 from %x to %x\n", syscall_getenvid(), who);
		
		syscall_mem_alloc(syscall_getenvid(), (u_int)str_send, PTE_V | PTE_R);
		strcpy(str_send, "Hello World!\n");

		ipc_send(who, 0, str_send, PTE_V | PTE_R);
		//user_panic("&&&&&&&&&&&&&&&&&&&&&&&&m");
	}

	for (;;) {
		writef("%x am waiting.....\n", syscall_getenvid());	

		i = ipc_recv(&who, str_recv, 0);
		writef("%x got String %s from %x.\n", syscall_getenvid(), str_recv, who);

		writef("%x got %d from %x\n", syscall_getenvid(), i, who);

		//user_panic("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&");
		if (i == 1) return;

		i++;
		writef("\n@@@@@send %d from %x to %x\n",i, syscall_getenvid(), who);
		ipc_send(who, i, str_recv, PTE_V | PTE_R);

		if (i == 1) {
			return;
		}
	}
	return;
}

