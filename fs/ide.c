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

int raid4_valid(u_int diskno) {
	// 0x200: the size of a sector: 512 bytes.
	int offset = 0;
	int abs_offset = 0, status;
	char command = 0;
	int ret;

	/* Step1: set IDE_ID. */
	// make C compiler use diskno as a memory address, not register.
	// Now I know why stack need to alloc 4 empty positions for arguments.
	ret = syscall_write_dev(&diskno, 0x13000010, 4);
	if (ret < 0) user_panic("Disk read panic^^^^");

	/* Step2: set read offset. */
	abs_offset = 0;
	ret = syscall_write_dev(&abs_offset, 0x13000000, 4);
	if (ret < 0) user_panic("Disk read panic^^^^");

	/* Step3: set command to read. */
	command = 0; // read command
	ret = syscall_write_dev(&command, 0x13000020, 1);
	if (ret < 0) user_panic("Disk read panic^^^^");

	/* Step4: read operation status. */
	ret = syscall_read_dev(&status, 0x13000030, 4);
	if (ret < 0) user_panic("Disk read panic^^^^");

	if (status == 0) return 0;
	else return 1;
}

char buffer[1024];
int raid4_write(u_int blockno, void *src) {
	/* Step0: check disk is bad? */
	int i, j, invalid_cnt = 0, valid[6];
	char *data = (char *)src;
	for (i = 1; i <= 5; i++) {
		// valid
		if (raid4_valid(i) == 1) {
			valid[i] = 1;
		}
		else {
			valid[i] = 0;
			invalid_cnt += 1;
		}
	}

	// begin to write
	/* Step1: calc check value. */
	for (j = 0; j < 512; j++) {
		buffer[j] = data[0*0x200 + j] ^ data[1*0x200 + j] ^ data[2*0x200 + j] ^ data[3*0x200 + j];
		buffer[512+j] = data[4*0x200 + j] ^ data[5*0x200 + j] ^ data[6*0x200 + j] ^ data[7*0x200 + j];
	}

	/* Step2: write data into disk. */
	for (i = 0; i < 8; i++) {
		if (valid[i % 4 + 1]) {
			ide_write(i % 4 + 1, blockno*2 + i/4, src + 0x200 * i, 1);
		}
	}

	/* Step3: write check code into disk. */
	if (valid[5]) {
		ide_write(5, blockno*2, buffer, 1);
		ide_write(5, blockno*2+1, buffer+512, 1);
	}

	return invalid_cnt;
}

int raid4_read(u_int blockno, void *dst) {
	/* Step0: check disk is bad? */
	int i, j, k, invalid_cnt = 0, valid[6], secno;
	char *data = (char *)dst;
	for (i = 1; i <= 5; i++) {
		// valid
		if (raid4_valid(i) == 1) {
			valid[i] = 1;
		}
		else {
			valid[i] = 0;
			invalid_cnt += 1;
		}
	}

	if (invalid_cnt > 1) {
		return invalid_cnt;
	}
	else if (invalid_cnt == 0) {
		/* Step1: read data from disk. */
		for (i = 0; i < 8; i++) {
			ide_read(i % 4 + 1, blockno*2 + i/4, dst + 0x200 * i, 1);
		}

		/* Step2.5: read check code from disk. */
		ide_read(5, blockno*2, buffer, 1);
		ide_read(5, blockno*2+1, buffer+512, 1);

		/* Step2: check code. */
		for (j = 0; j < 512; j++) {
			if (buffer[j] != (data[0*0x200 + j] ^ data[1*0x200 + j] ^ data[2*0x200 + j] ^ data[3*0x200 + j])
				|| buffer[512+j] != (data[4*0x200 + j] ^ data[5*0x200 + j] ^ data[6*0x200 + j] ^ data[7*0x200 + j]) ) {
					return -1;
				}
		}
		return 0;
	}
	else {
		// invalid_cnt == 1
		/* Step1: read data from disk. */
		for (i = 0; i < 8; i++) {
			if (valid[i % 4 + 1]) {
				ide_read(i % 4 + 1, blockno*2 + i/4, dst + 0x200 * i, 1);
			}
		}

		if (!valid[5]) {
			// check disk is bad.
			return 1;
		}
		else {
			/* Step2: read check code from disk. */
			ide_read(5, blockno*2, buffer, 1);
			ide_read(5, blockno*2+1, buffer+512, 1);

			// other disk is bad.
			int diskno_bad = 0;
			for (i = 1; i <= 4; i++) {
				if (!valid[i]) {
					diskno_bad = i;
				}
			}

			for (i = 0; i < 2; i++) {
				for (j = 0; j < 512; j++) {
					secno = i * 4 + diskno_bad - 1;
					data[secno * 0x200 + j] = buffer[i * 0x200 + j];
					for (k = 1; k <= 4; k++) {
						if (k == diskno_bad) continue;
						data[secno * 0x200 + j] ^= data[(i * 4 + k - 1) * 0x200 + j];
					}
				}
			}
			return 1;
		}
	}
}