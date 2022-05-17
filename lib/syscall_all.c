#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern char *KERNEL_SP;
extern struct Env *curenv;

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current environment. This function will never return.
 */
/*** exercise 4.6 ***/
void sys_yield(void)
{
	/* Step1: copy from KERNAL_SP to TIMESTACK */
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
		  (void *)TIMESTACK - sizeof(struct Trapframe), 
		  sizeof(struct Trapframe));

	/* Step2: yield right to execute. */
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
/*** exercise 4.12 ***/
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;
	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}
	env->env_pgfault_handler = func;
	env->env_xstacktop = xstacktop;
	
	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
/*** exercise 4.3 ***/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// printf("SYS_MEM_ALLOC: envid = %d, va = %x, perm = %012b.\n", envid, va, perm);
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;
	/* Step1: Round va to the multiples of BY2PG. */
	va = ROUNDDOWN(va, BY2PG);

	/* Step2: Check whether VA is below UTOP. */
	if (va >= UTOP) return -E_INVAL;

	/* Step3: Check perm. */
	if ((perm & PTE_V) == 0 || (perm & PTE_COW) != 0) return -E_INVAL;

	/* Step4: Assign env to the env of envid. */
	if ((ret = envid2env(envid, &env, 0)) < 0) {
		// ERROR when get env.
		return ret;
	}

	/* Step5: Alloc a page. */
	if ((ret = page_alloc(&ppage)) < 0)
		return ret;

	/* Step6: insert the page into user address space. */
	if ((ret = page_insert(env->env_pgdir, ppage, va, perm)) < 0)
		return ret;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_mem_alloc.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
/*** exercise 4.4 ***/
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte, *ppte_M;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

    // your code here
	/* Step1: Check va position. */
	if (round_srcva >= UTOP || round_dstva >= UTOP) {
		return -E_INVAL;
	}

	/* Step3: Check perm. */
	// To allow set PTE_COW, delete " || (perm & PTE_COW) != 0"
	if ((perm & PTE_V) == 0)
		return -E_INVAL;

	/* Step4: Assign env to the env of envid. */
	if ((ret = envid2env(srcid, &srcenv, 0)) < 0) {
		// ERROR when get env.
		return ret;
	}

	if ((ret = envid2env(dstid, &dstenv, 0)) < 0) {
		return ret;
	}

	/* Step5: query physical address of srcva. */
	if ((ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte)) == NULL) {
		/* srcva don't match any physical memory space. */
		return -E_BAD_ENV;
	}

	if ((*ppte & PTE_R) == 0 && (perm & PTE_R) != 0) {
		// ppte read-only, but perm allows write. this will cause error.
		return -E_INVAL;
	}

	/* Step6: Map srcva's physical address to dstva. */
	if((ret = pgdir_walk(dstenv->env_pgdir, round_dstva, 1, &ppte_M)) < 0) {
		return ret;
	}
	*ppte_M = PTE_ADDR(*ppte) | perm;

	ppage->pp_ref += 1; // increase pp_ref, IMPORTANT!!

	return ret;
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
/*** exercise 4.5 ***/
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret = 0;
	struct Env *env;

	/* Step1: Check va position. */
	va = ROUNDDOWN(va, BY2PG);
	if (va >= UTOP) {
		return -E_INVAL;
	}

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		// ERROR when get env.
		return ret;
	}

	page_remove(env->env_pgdir, va);

	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
/*** exercise 4.8 ***/
int sys_env_alloc(void)
{
	// Your code here.
	int r;
	struct Env *e;

	/* Step1: Allocate an env and set appropriate value to it. */
	/* Values include: envid, status, parent id, some value of Trapframe */
	/* 也执行env_setup_vm函数，分配了页目录，并映射了一些必要的空间。*/
	env_alloc(&e, curenv->env_id);
	
	// error.
	bcopy((void *)(KERNEL_SP - sizeof(struct Trapframe)),
		  (void *)(&(e->env_tf)), 
		  sizeof(struct Trapframe));

	// printf("Now curenvID = %d, EPC = %x, childID = %d, child_epc = %x\n", curenv->env_id, curenv->env_tf.cp0_epc, e->env_id, e->env_tf.cp0_epc);
	
	e->env_tf.pc = e->env_tf.cp0_epc;

	e->env_tf.regs[2] = 0; // v0: indicates that we returns 0.

	e->env_status = ENV_NOT_RUNNABLE;

	e->env_pri = curenv->env_pri;

	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
/*** exercise 4.14 ***/
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct Env *env;
	int ret;
	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}
	// if is not in the following 3 states: RUNNABLE, NOT_RUNNABLE, FREE
	if (env->env_status != ENV_RUNNABLE && env->env_status != ENV_NOT_RUNNABLE && env->env_status != ENV_FREE) {
		return -E_INVAL;
	}
	// printf("sys_set_env_status: envid = %d, env_pc = %x\n", envid, env->env_tf.pc);
	if (env->env_status == ENV_NOT_RUNNABLE && status == ENV_RUNNABLE) {
		LIST_INSERT_HEAD(env_sched_list, env, env_sched_link);
	}
	env->env_status = status;
	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
/*** exercise 4.7 ***/
void sys_ipc_recv(int sysno, u_int dstva)
{
	curenv->env_ipc_recving = 1; // prepare to recv data.
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sys_yield(); // yield the right to execute
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
/*** exercise 4.7 ***/
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{

	int r;
	struct Env *e;
	struct Page *p;
	if ((r = envid2env(envid, &e, 0)) < 0) {
		printf("ipc_send: envid2env error!\n");
		return r;
	}
	if (e->env_ipc_recving == 0) {
		return -E_IPC_NOT_RECV;
	}
	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	
	if (srcva != 0) {
		// means need to share mem.
		if ((r = sys_mem_map(sysno, curenv->env_id, srcva, e->env_id, e->env_ipc_dstva, perm)) < 0) {
			printf("ipc_send: sys_mem_map error!\n");
			return r;
		}
	}

	e->env_status = ENV_RUNNABLE;
	return 0;
}
