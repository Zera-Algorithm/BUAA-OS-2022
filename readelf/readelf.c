/* This is a simplefied ELF reader.
 * You can contact me if you find any bugs.
 *
 * Luming Wang<wlm199558@126.com>
 */

#include "kerelf.h"
#include <stdio.h>
/* Overview:
 *   Check whether it is a ELF file.
 *
 * Pre-Condition:
 *   binary must longer than 4 byte.
 *
 * Post-Condition:
 *   Return 0 if `binary` isn't an elf. Otherwise
 * return 1.
 */
int is_elf_format(u_char *binary)
{
        Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
        if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
                ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
                ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
                ehdr->e_ident[EI_MAG3] == ELFMAG3) {
                return 1;
        }

        return 0;
}

/* Overview:
 *   read an elf format binary file. get ELF's information
 *
 * Pre-Condition:
 *   `binary` can't be NULL and `size` is the size of binary.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, output address of every section in ELF.
 */

/*
    Exercise 1.2. Please complete func "readelf". 
*/
int cmp(void *a, void* b) {
	Elf32_Phdr *x = (Elf32_Phdr *)a;
	Elf32_Phdr *y = (Elf32_Phdr *)b;
	return (x->p_vaddr - y->p_vaddr);
}

int readelf(u_char *binary, int size)
{
        Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;

        int Nr;

        Elf32_Phdr *phdr = NULL;
        Elf32_Half ph_entry_count;
        Elf32_Half ph_entry_size;


        // check whether `binary` is a ELF file.
        if (size < 4 || !is_elf_format(binary)) {
                printf("not a standard elf format\n");
                return 0;
        }

        // get section table addr, section header number and section header size.
	Elf32_Off phoff = ehdr->e_phoff;
	ph_entry_count = ehdr->e_phnum;
	ph_entry_size = ehdr->e_phentsize;
        // for each section header, output section number and section addr. 
        // hint: section number starts at 0.
	phdr = (Elf32_Phdr *)(binary + phoff);
	int i;
	for (i = 0; i < ph_entry_count; i++) {
		(phdr + i) -> p_type = (Elf32_Word)i;
	}
	// qsort( (void *)phdr, (size_t)ph_entry_count, sizeof(Elf32_Phdr), cmp);
	
	for (i = 0; i < ph_entry_count; i++) {
		Elf32_Addr l1 = phdr[i].p_vaddr;
		Elf32_Word memsz1 = phdr[i].p_memsz;
		Elf32_Addr r1 = l1 + memsz1 - 1;
		
		if (i+1 < ph_entry_count) {
			Elf32_Addr l2 = phdr[i+1].p_vaddr;
        	Elf32_Word memsz2 = phdr[i+1].p_memsz;
        	Elf32_Addr r2 = l2 + memsz2 - 1;

			if( (r1 & (0xfffff000)) == (l2 & (0xfffff000)) ) {
				if (r1 < l2) {
					// Overlay
					printf("Overlay at page va : 0x%x\n", l1);
				}
				else {
					// Conflict
					printf("Conflict at page va : 0x%x\n", l1);
				}
				i += 1;
				continue;
			}
		}
		printf("%d:0x%x,0x%x\n", phdr[i].p_type, phdr[i].p_filesz, phdr[i].p_memsz);
	}
    return 0;
}

