// implement fork from user space

#include "lib.h"
#include <mmu.h>
#include <env.h>


/* ----------------- help functions ---------------- */

/* Overview:
 * 	Copy `len` bytes from `src` to `dst`.
 *
 * Pre-Condition:
 * 	`src` and `dst` can't be NULL. Also, the `src` area
 * 	 shouldn't overlap the `dest`, otherwise the behavior of this
 * 	 function is undefined.
 */
void user_bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	//	writef("~~~~~~~~~~~~~~~~ src:%x dst:%x len:%x\n",(int)src,(int)dst,len);
	max = dst + len;

	// copy machine words while possible
	if (((int)src % 4 == 0) && ((int)dst % 4 == 0)) {
		while (dst + 3 < max) {
			*(int *)dst = *(int *)src;
			dst += 4;
			src += 4;
		}
	}

	// finish remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst = *(char *)src;
		dst += 1;
		src += 1;
	}

	//for(;;);
}

/* Overview:
 * 	Sets the first n bytes of the block of memory
 * pointed by `v` to zero.
 *
 * Pre-Condition:
 * 	`v` must be valid.
 *
 * Post-Condition:
 * 	the content of the space(from `v` to `v`+ n)
 * will be set to zero.
 */
void user_bzero(void *v, u_int n)
{
	char *p;
	int m;

	p = v;
	m = n;

	while (--m >= 0) {
		*p++ = 0;
	}
}
/*--------------------------------------------------------------*/

/* Overview:
 * 	Custom page fault handler - if faulting page is copy-on-write,
 * map in our own private writable copy.
 *
 * Pre-Condition:
 * 	`va` is the address which leads to a TLBS exception.
 *
 * Post-Condition:
 *  Launch a user_panic if `va` is not a copy-on-write page.
 * Otherwise, this handler should map a private writable copy of
 * the faulting page at correct address.
 */
/*** exercise 4.13 ***/

// Current Stack is UXSTACK!
static void
pgfault(u_int va)
{
	u_int tmp = (u_int)USTACKTOP; // a page of invalid mem.
	int r;
	int env_id = syscall_getenvid();
	u_int pn = (va >> 12) & 0xfffff;
	u_int perm = (((u_int)((*vpt)[pn])) & 0xfff);
	
	// writef("Pgfault: Happens at VA = %x.\n", va);

	// writef("1. Stack saved ra = %x.\n", *(int *)(0x7f3fdfbc));

	if ((perm & PTE_COW) == 0) {
		user_panic("Error: PAGE FAULT Happens when PTE_COW is not set!!");
	}
	// alloc a page to tmp address.

	// writef("Pgfault: curenv->envid = %d.\n", env_id);
	
	// writef("2. Stack saved ra = %x.\n", *(int *)(0x7f3fdfbc));
	// don't use env->env_id, it's not correct! Pgfault may happens in syscall_getenvid.
	r = syscall_mem_alloc(env_id, tmp, perm ^ PTE_COW);
		
	// writef("before copy: Stack saved ra = %x.\n", *(int *)(0x7f3fdfbc));

	// writef("The result of tmp alloc: ret = %d.\n", r);
	// copy content of va to tmp address.
	
	// writef("Pgfault: copy COW page to tmp position(%08x).\n", tmp);
	
	user_bcopy((void *)(va & 0xfffff000), (void *)tmp, BY2PG);
	syscall_mem_unmap(env_id, (va & 0xfffff000)); // When unmap, it also invalidates tlb table items, that's also what we need.
	
	// (*vpt)[pn] = (*vpt)[tmp >> 12]; // copy perm and PA, Access by User level's pgtable, ERROR!

	// You must map using syscall_mem_map AS FOLLOWS. Or your page could be deleted by page_remove.
	syscall_mem_map(env_id, tmp & 0xfffff000, env_id, pn << 12, perm ^ PTE_COW);

	// writef("VA(%08x) maps to PA(%08x).\n", va & 0xfffff000, (*vpt)[tmp >> 12] & 0xfffff000);
	syscall_mem_unmap(env_id, tmp); // release the tmp address Mapping.
	
	// writef("after copy: Stack saved ra = %x.\n", *(int *)(0x7f3fdfbc));

	//	writef("fork.c:pgfault():\t va:%x\n",va);
	
	//map the new page at a temporary place

	//copy the content

	//map the page on the appropriate place

	//unmap the temporary place

}

/* Overview:
 * 	Map our virtual page `pn` (address pn*BY2PG) into the target `envid`
 * at the same virtual address.
 *
 * Post-Condition:
 *  if the page is writable or copy-on-write, the new mapping must be
 * created copy on write and then our mapping must be marked
 * copy on write as well. In another word, both of the new mapping and
 * our mapping should be copy-on-write if the page is writable or
 * copy-on-write.
 *
 * Hint:
 * 	PTE_LIBRARY indicates that the page is shared between processes.
 * A page with PTE_LIBRARY may have PTE_R at the same time. You
 * should process it correctly.
 */
/*** exercise 4.10 ***/
static void
duppage(u_int envid, u_int pn)
{
	u_int addr;
	u_int perm;
	Pte item;
	addr = (pn << 12);
	int ret;
	// if the pgtable is not in page dir, then skip. // not necessary for mapping.
	if ( (*vpd)[(pn >> 10) & 0x3FF] & PTE_V) {
		item = (*vpt)[pn]; // pre-condition: pn's pgtable is valid.
		if (item & PTE_V) {
			// need the page is Valid.
			if ((item & PTE_R) == 0 || (item & PTE_COW) != 0 || (item & PTE_LIBRARY) != 0) {
				ret = syscall_mem_map(env->env_id, addr, envid, addr, item & 0xfff);
			}
			else if ((item & PTE_R) != 0) {
				(*vpt)[pn] = (item | PTE_COW);
				ret = syscall_mem_map(env->env_id, addr, envid, addr, (item | PTE_COW) & 0xfff);
			}
			// writef("duppage -> syscall_mem_map: ret = %d, addr = %x\n", ret, pn << 12);
		}
	}
	//	user_panic("duppage not implemented");
}

/* Overview:
 * 	User-level fork. Create a child and then copy our address space
 * and page fault handler setup to the child.
 *
 * Hint: use vpd, vpt, and duppage.
 * Hint: remember to fix "env" in the child process!
 * Note: `set_pgfault_handler`(user/pgfault.c) is different from
 *       `syscall_set_pgfault_handler`.
 */
/*** exercise 4.9 4.15***/
extern void __asm_pgfault_handler(void);
int
fork(void)
{
	// Your code here.
	u_int newenvid, tempEnvid;
	extern struct Env *envs;
	extern struct Env *env;
	u_int i, addr;

	set_pgfault_handler(pgfault); // must set before syscall_env_alloc, since YOU should pass the __pgfault_handler to the child.
	//The parent installs pgfault using set_pgfault_handler

	// writef("It's user space's fork!\n");

	newenvid = syscall_env_alloc();
	
	// writef("Finish user env_alloc syscall.\n");
	// writef("My envid = %d, child is %d.\n", syscall_getenvid(), newenvid);

	//alloc a new alloc
	/* If this is child process. */
	if (newenvid == 0) {
		// writef("This is child space.\n");

		/* child process: set env to its PCB */
		tempEnvid = syscall_getenvid();

		// writef("Child knows its id = %d.\n", tempEnvid);
		i = (tempEnvid & ((1<<10)-1));
		env = envs + i;

		// writef("child process finish fork. sys_env_alloc@ret_value = %d.\n", newenvid);
	}
	else { // parent process
		// Copy COW Memory.
		// writef("duppaging...\n");

		for (addr = 0; addr < USTACKTOP; addr += BY2PG) {
			duppage(newenvid, addr >> 12);
		}

		// writef("malloc for UXSTACK...\n");
		syscall_mem_alloc(newenvid, UXSTACKTOP-BY2PG, PTE_R | PTE_V);
		
		// writef("set Handler...\n");
		syscall_set_pgfault_handler(newenvid, __asm_pgfault_handler, UXSTACKTOP);
		
		syscall_set_env_status(newenvid, ENV_RUNNABLE);
		// writef("finish father space.\n");
	}

	return newenvid;
}

// Challenge!
int
sfork(void)
{
	user_panic("sfork not implemented");
	return -E_INVAL;
}
