#include <asm/asm.h>
#include <pmap.h>
#include <env.h>
#include <printf.h>
#include <kclock.h>
#include <trap.h>

void mips_init()
{
	printf("init.c:\tmips_init() is called\n");
	mips_detect_memory();
	mips_vm_init();
	page_init();
	env_init();
	// for lab3-2-exam local test
	ENV_CREATE_PRIORITY(user_A, 2);
	ENV_CREATE_PRIORITY(user_B, 1);
	trap_init();
	kclock_init();
	panic("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
	while(1);
	panic("init.c:\tend of mips_init() reached!");
}


void bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	max = dst + len;
	// copy machine words while possible
	while (dst + 3 < max)
	{
		*(int *)dst = *(int *)src;
		dst+=4;
		src+=4;
	}
	// finish remaining 0-3 bytes
	while (dst < max)
	{
		*(char *)dst = *(char *)src;
		dst+=1;
		src+=1;
	}
}

void bzero(void *b, size_t len)
{
	void *max;
	/* debug */ int i = 0;

	max = b + len;
	
	//printf("init.c:\tzero from %x to %x\n",(int)b,(int)max);

	// zero machine words while possible

	/* ZRP bug fix: possible bug: b is not aligned with 4. */
	while ( ((u_long)(b) & 0x4) != 0 ) {
		*(char *)b = 0;
		b+=1;
	}

	while (b + 3 < max)
	{
		*(int *)b = 0;
		/* debug */
		if (i == 0) {
			// printf("First Zero Write: %x\n", b);
		}
		i = 1;
		/* end debug */
		b+=4;
	}

	// finish remaining 0-3 bytes
	while (b < max)
	{
		*(char *)b++ = 0;
	}

}
