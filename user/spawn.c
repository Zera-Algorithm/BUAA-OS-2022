#include "lib.h"
#include <mmu.h>
#include <env.h>
#include <kerelf.h>

#define debug 0
#define TMPPAGE		(BY2PG)
#define TMPPAGETOP	(TMPPAGE+BY2PG)

int
init_stack(u_int child, char **argv, u_int *init_esp)
{
	int argc, i, r, tot;
	char *strings;
	u_int *args;

	// Count the number of arguments (argc)
	// and the total amount of space needed for strings (tot)
	tot = 0;
	for (argc=0; argv[argc]; argc++)
		tot += strlen(argv[argc])+1;

	// Make sure everything will fit in the initial stack page
	if (ROUND(tot, 4)+4*(argc+3) > BY2PG)
		return -E_NO_MEM;

	// Determine where to place the strings and the args array
	strings = (char*)TMPPAGETOP - tot;
	args = (u_int*)(TMPPAGETOP - ROUND(tot, 4) - 4*(argc+1));

	if ((r = syscall_mem_alloc(0, TMPPAGE, PTE_V|PTE_R)) < 0)
		return r;
	// Replace this with your code to:
	//
	//	- copy the argument strings into the stack page at 'strings'
	char *ctemp,*argv_temp;
	u_int j;
	ctemp = strings;
	for(i = 0;i < argc; i++)
	{
		argv_temp = argv[i];
		for(j=0;j < strlen(argv[i]);j++)
		{
			*ctemp = *argv_temp;
			ctemp++;
			argv_temp++;
		}
		*ctemp = 0;
		ctemp++;
	}
	//	- initialize args[0..argc-1] to be pointers to these strings
	//	  that will be valid addresses for the child environment
	//	  (for whom this page will be at USTACKTOP-BY2PG!).
	ctemp = (char *)(USTACKTOP - TMPPAGETOP + (u_int)strings);
	for(i = 0;i < argc;i++)
	{
		args[i] = (u_int)ctemp;
		ctemp += strlen(argv[i])+1;
	}
	//	- set args[argc] to 0 to null-terminate the args array.
	ctemp--;
	args[argc] = ctemp;
	//	- push two more words onto the child's stack below 'args',
	//	  containing the argc and argv parameters to be passed
	//	  to the child's umain() function.
	u_int *pargv_ptr;
	pargv_ptr = args - 1;
	*pargv_ptr = USTACKTOP - TMPPAGETOP + (u_int)args;
	pargv_ptr--;
	*pargv_ptr = argc;
	//
	//	- set *init_esp to the initial stack pointer for the child
	//
	*init_esp = USTACKTOP - TMPPAGETOP + (u_int)pargv_ptr;
//	*init_esp = USTACKTOP;	// Change this!

	if ((r = syscall_mem_map(0, TMPPAGE, child, USTACKTOP-BY2PG, PTE_V|PTE_R)) < 0)
		goto error;
	if ((r = syscall_mem_unmap(0, TMPPAGE)) < 0)
		goto error;

	return 0;

error:
	syscall_mem_unmap(0, TMPPAGE);
	return r;
}

int usr_is_elf_format(u_char *binary){
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	if (ehdr->e_ident[0] == ELFMAG0 &&
        ehdr->e_ident[1] == ELFMAG1 &&
        ehdr->e_ident[2] == ELFMAG2 &&
        ehdr->e_ident[3] == ELFMAG3) {
        return 1;
    }   

    return 0;
}

int 
usr_load_elf(int child_envid, u_char *binary, int size,
		int (*map)(int child_envid, u_long va, u_int32_t sgsize,
			u_char *bin, u_int32_t bin_size)) {
	//Hint: maybe this function is useful 
	//      If you want to use this func, you should fill it ,it's not hard
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	Elf32_Phdr *phdr = NULL;
	/* As a loader, we just care about segment,
         * so we just parse program headers.
         */
	u_char *ptr_ph_table = NULL;
	Elf32_Half ph_entry_count;
	Elf32_Half ph_entry_size;
	int r;

	ptr_ph_table = binary + ehdr->e_phoff;
	ph_entry_count = ehdr->e_phnum;
	ph_entry_size = ehdr->e_phentsize;

	while (ph_entry_count--) {
		phdr = (Elf32_Phdr *)ptr_ph_table;
		if (phdr->p_type == PT_LOAD) {
			/* Real map all section at correct virtual address.Return < 0 if error. */
			/* Hint: Call the callback function you have achieved before. */
			// printf("Load_elf: Va=%x, memsize=%x, filesize=%x.\n", phdr->p_vaddr, phdr->p_memsz, phdr->p_filesz);
			if( (r = (*map)(child_envid, phdr->p_vaddr, phdr->p_memsz, 
					binary + phdr->p_offset, phdr->p_filesz)) < 0) {
							return r;
			}
			// printf("Load_elf: Success.\n");
		}
		ptr_ph_table += ph_entry_size;
	}
	return 0;
}

int usr_load_icode_mapper(int child_envid, u_long va, u_int32_t sgsize, 
						  u_char *bin, u_int32_t bin_size) {
    struct Page *p = NULL;
    u_long i;
    int r;
    u_long offset = va - ROUNDDOWN(va, BY2PG);
    u_long nowVA = ROUNDDOWN(va, BY2PG);
	Pte *pte;
	
    /* Step 1: load all content of bin into memory. */
	// 为第一页分配空间
	r = syscall_mem_alloc(0, TMPPAGE, PTE_R | PTE_V); 
	// 先给自己分配空间，因为只有自己的内存空间才能被自己访问
	if (r < 0) return r;
	
    /* Condition: |---(----)---| Only One Page */
    if (offset + bin_size <= BY2PG) {
        user_bcopy(bin, (void *)(TMPPAGE + offset), bin_size);
		syscall_mem_map(0, TMPPAGE, child_envid, nowVA, PTE_V | PTE_R);
		syscall_mem_unmap(0, TMPPAGE); // 释放自己的内存空间

		i += BY2PG;
		nowVA += BY2PG;
    }
    else {
        /* Condition: |----(----|---------|----)---| */
        user_bcopy(bin, (void *)(TMPPAGE + offset), BY2PG - offset);
		syscall_mem_map(0, TMPPAGE, child_envid, nowVA, PTE_V | PTE_R);
		syscall_mem_unmap(0, TMPPAGE); // 释放自己的临时内存空间

        for (i = BY2PG - offset, nowVA += BY2PG; i < bin_size; i += BY2PG, nowVA += BY2PG) {
            /* Hint: You should alloc a new page. */
			r = syscall_mem_alloc(0, TMPPAGE, PTE_R | PTE_V); 
			if (r < 0) return r;
            
			if (i + BY2PG >= bin_size) {
                /* 1.Last Page */
                user_bcopy(bin+i, (void *)TMPPAGE, bin_size - i);
            }
            else {
                /* Not Last Page */
                user_bcopy(bin+i, (void *)TMPPAGE, BY2PG);
            }
			syscall_mem_map(0, TMPPAGE, child_envid, nowVA, PTE_V | PTE_R);
			syscall_mem_unmap(0, TMPPAGE); // 释放自己的临时内存空间
        }
    }
    /* Step 2: alloc pages to reach `sgsize` when `bin_size` < `sgsize`.
     * hint: variable `i` has the value of `bin_size` now! */
    while (i < sgsize) {
        r = syscall_mem_alloc(0, TMPPAGE, PTE_R | PTE_V); 
		if (r < 0) return r;

        syscall_mem_map(0, TMPPAGE, child_envid, nowVA, PTE_V | PTE_R);
		syscall_mem_unmap(0, TMPPAGE); // 释放自己的临时内存空间

        i += BY2PG;
        nowVA += BY2PG;
    }
    return 0;
}

int spawn(char *prog, char **argv)
{
	u_char elfbuf[512];
	int r;
	int fd;
	u_int child_envid;
	int size, text_start;
	u_int i, *blk;
	u_int esp;
	Elf32_Ehdr* elf;
	Elf32_Phdr* ph;
	struct Stat stat;
	// Note 0: some variable may be not used,you can cancel them as you like
	// Step 1: Open the file specified by `prog` (prog is the path of the program)
	if((r=open(prog, O_RDONLY))<0){
		// user_panic("spawn ::open line 102 RDONLY wrong !\n");
		writef("spawn ::open line 102 RDONLY wrong !\n");
		return r;
	}
	// Your code begins here
	// Before Step 2 , You had better check the "target" spawned is a execute bin

	fd = r;
	read_map(fd, 0, &blk);
	fstat(fd, &stat);
	size = stat.st_size;

	if (size < 4 || !usr_is_elf_format((u_char *)blk)) {
		// user_panic("the binary file isn't an executable program.\n");
		writef("the binary file isn't an executable program.\n");
		return -E_INVAL;
	}

	// Step 2: Allocate an env (Hint: using syscall_env_alloc())
	child_envid = syscall_env_alloc();
	if (child_envid < 0) {
		user_panic("Error when syscall_env_alloc!\n");
	}

	// Step 3: Using init_stack(...) to initialize the stack of the allocated env
	init_stack(child_envid, argv, &esp);
	
	// Step 3: Map file's content to new env's text segment
	usr_load_elf(child_envid, (u_char *)blk, size, usr_load_icode_mapper);
	//        Hint 1: what is the offset of the text segment in file? try to use objdump to find out.
	//        Hint 2: using read_map(...)
	//		  Hint 3: Important!!! sometimes ,its not safe to use read_map ,guess why 
	//				  If you understand, you can achieve the "load APP" with any method
	
	// Note1: Step 1 and 2 need sanity check. In other words, you should check whether
	//       the file is opened successfully, and env is allocated successfully.
	
	// Note2: You can achieve this func in any way ，remember to ensure the correctness
	//        Maybe you can review lab3 
	// Your code ends here

	// 系统提供给用户进程对envs数组和pages的完全读写权限.
	struct Trapframe *tf;
	writef("\n::::::::::spawn size : %x  sp : %x::::::::\n",size,esp);
	tf = &(envs[ENVX(child_envid)].env_tf);
	tf->pc = UTEXT; // 设置起始执行位置为UTEXT段的开始
	tf->regs[29]=esp; // 设置esp


	// Share memory
	// 向子进程共享PTE_LIBRARY的页
	u_int pdeno = 0;
	u_int pteno = 0;
	u_int pn = 0;
	u_int va = 0;
	for(pdeno = 0;pdeno<PDX(UTOP);pdeno++)
	{
		if(!((* vpd)[pdeno]&PTE_V))
			continue;
		for(pteno = 0;pteno<=PTX(~0);pteno++)
		{
			pn = (pdeno<<10)+pteno;
			if(((* vpt)[pn]&PTE_V)&&((* vpt)[pn]&PTE_LIBRARY))
			{
				va = pn*BY2PG;

				if((r = syscall_mem_map(0,va,child_envid,va,(PTE_V|PTE_R|PTE_LIBRARY)))<0)
				{

					writef("va: %x   child_envid: %x   \n",va,child_envid);
					user_panic("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
					return r;
				}
			}
		}
	}

	// 设置spawn的进程为可执行的进程。
	if((r = syscall_set_env_status(child_envid, ENV_RUNNABLE)) < 0)
	{
		writef("set child runnable is wrong\n");
		return r;
	}
	return child_envid;		

}

int
spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}


