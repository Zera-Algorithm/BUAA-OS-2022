/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.
// 	secno: start sector number.
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read.
//
// Post-Condition:
// 	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0;
	int abs_offset = 0, status;
	char command = 0;
	int ret;
		
	writef("diskno: %d\n", diskno);
	/* Step1: set IDE_ID. */
	// make C compiler use diskno as a memory address, not register.
	// Now I know why stack need to alloc 4 empty positions for arguments.
	ret = syscall_write_dev(&diskno, 0x13000010, 4);
	if (ret < 0) user_panic("Disk read panic^^^^");

	while (offset_begin + offset < offset_end) {
		// Your code here
		// error occurred, then panic.
		
		/* Step2: set read offset. */
		abs_offset = offset_begin + offset;
		ret = syscall_write_dev(&abs_offset, 0x13000000, 4);
		if (ret < 0) user_panic("Disk read panic^^^^");

		/* Step3: set command to read. */
		command = 0; // read command
		ret = syscall_write_dev(&command, 0x13000020, 1);
		if (ret < 0) user_panic("Disk read panic^^^^");

		/* Step4: read operation status. */
		ret = syscall_read_dev(&status, 0x13000030, 4);
		if (ret < 0) user_panic("Disk read panic^^^^");

		/* Step5: read sector data to dst. */
		ret = syscall_read_dev(dst + offset, 0x13004000, 0x200);
		if (ret < 0) user_panic("Disk read panic^^^^");

		offset += 0x200;
	}
}


// Overview:
// 	write data to IDE disk.
//
// Parameters:
//	diskno: disk number.
//	secno: start sector number.
// 	src: the source data to write into IDE disk.
//	nsecs: the number of sectors to write.
//
// Post-Condition:
//	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
	// Your code here
	int offset_begin = secno * 0x200;
	int offset_end = offset_begin + nsecs * 0x200;
	int offset = 0;
	int abs_offset = 0, status;
	char command = 0;
	int ret;

	// DO NOT DELETE WRITEF !!!
	writef("diskno: %d\n", diskno);

	/* Step1: set IDE_ID. */
	ret = syscall_write_dev(&diskno, 0x13000010, 4);
	if (ret < 0) user_panic("Disk write panic^^^^");

	while (offset_begin + offset < offset_end) {
		// Your code here
		// error occurred, then panic.
		
		/* Step2: set write offset. */
		abs_offset = offset_begin + offset;
		ret = syscall_write_dev(&abs_offset, 0x13000000, 4);
		if (ret < 0) user_panic("Disk write panic^^^^");

		/* Step3: write sector data to disk buffer. */
		ret = syscall_write_dev(src + offset, 0x13004000, 0x200);
		if (ret < 0) user_panic("Disk write panic^^^^");

		/* Step3: set command to read. */
		command = 1; // write command
		ret = syscall_write_dev(&command, 0x13000020, 1);
		if (ret < 0) user_panic("Disk write panic^^^^");

		/* Step4: read operation status. */
		ret = syscall_read_dev(&status, 0x13000030, 4);
		if (ret < 0) user_panic("Disk write panic^^^^");

		offset += 0x200;
	}
}

void raid0_write(u_int secno, void *src, u_int nsecs) {
	u_int secno_now;
	for (secno_now = secno; secno_now < secno + nsecs; secno_now++) {
		if (secno_now % 2 == 0) {
			ide_write(1, secno_now/2, src + (secno_now-secno) * 0x200, 1);
		}
		else {
			ide_write(2, secno_now/2, src + (secno_now-secno) * 0x200, 1);
		}
	}
}

void raid0_read(u_int secno, void *dst, u_int nsecs) {
	u_int secno_now;
	for (secno_now = secno; secno_now < secno + nsecs; secno_now++) {
		if (secno_now % 2 == 0) {
			ide_read(1, secno_now/2, dst + (secno_now-secno) * 0x200, 1);
			// writef("read disk1, secno = %u\n", secno_now/2);
		}
		else {
			ide_read(2, secno_now/2, dst + (secno_now-secno) * 0x200, 1);
			// writef("read disk2, secno = %u\n", secno_now/2);
		}
	}
}

int time_read() {
	int time;
	syscall_read_dev(&time, 0x15000000, 1); // trigger update
	syscall_read_dev(&time, 0x15000010, 4);
	return time;
}
