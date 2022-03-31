#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>


.text
LEAF(_my_getchar)
	addiu sp, sp, -16
	sw v1, 0(sp)
	sw a0, 4(sp)
	sw ra, 8(sp)
	
	lui v0, 0xb000 # kseg0
	ori v0, 0x0000
	lb v1, 0(v0)
	
	# call _my_putchar
	move a0, v1
	jal _my_putchar
	
	# sb v1, 0(v0) # to write back
	move v0, v1 # return is in v0
	
	lw ra, 8(sp)
	lw a0, 4(sp)
	lw v1, 0(sp)
	addiu sp, sp, 16
	jr ra
END(_my_getchar)



LEAF(_my_putchar)
    addiu sp, sp, -8
    sw v0, 0(sp)

    lui v0, 0xb000
    ori v0, 0x0000
    sb a0, 0(v0)
    move v0, v1

    lw v0, 0(sp)
    addiu sp, sp, 8
    jr ra
END(_my_putchar)




LEAF(_my_exit)
	lui v0, 0xb000
	ori v0, 0x0010
	lw v1, 0(v0)
END(_my_exit)
