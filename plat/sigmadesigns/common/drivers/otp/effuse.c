
#include <bl_common.h>
#include <mmio.h>
#include <platform_def.h>
#include <sd_private.h>

#ifdef SD_OTP_FUSE_BASE
# define FC_BASE		SD_OTP_FUSE_BASE
#else
# error "Unknow Fuse base"
#endif

#define FC_WRITE_LOCK_REG	(FC_BASE + 0x0)
#define FC_WRITE_ADDR_REG	(FC_BASE + 0x4)
#define FC_WRITE_STATUS_REG	(FC_BASE + 0x8)
#define FC_WRITE_DATA0_REG	(FC_BASE + 0xc)
#define FC_WRITE_DATA1_REG	(FC_BASE + 0x10)
#define FC_WRITE_DATA2_REG	(FC_BASE + 0x14)
#define FC_WRITE_DATA3_REG	(FC_BASE + 0x18)
#define FC_WRITE_PROT_REG	(FC_BASE + 0x1c)
#define FC_READ_ADDR_REG	(FC_BASE + 0x20)
#define FC_READ_STATUS_REG	(FC_BASE + 0x24)
#define FC_READ_DATA0_REG	(FC_BASE + 0x28)
#define FC_READ_DATA1_REG	(FC_BASE + 0x2c)
#define FC_READ_DATA2_REG	(FC_BASE + 0x30)
#define FC_READ_DATA3_REG	(FC_BASE + 0x34)
#define FC_READ_PROT_REG	(FC_BASE + 0x38)
#define FC_DEBUG_LOCK_REG	(FC_BASE + 0x3c)
#define FC_DEBUG_CONTROL_0_REG	(FC_BASE + 0x40)
#define FC_DEBUG_CONTROL_1_REG	(FC_BASE + 0x44)
#define FC_DEBUG_STATUS_REG	(FC_BASE + 0x48)
#define FC_DEBUG_TEST_STATUS_REG (FC_BASE + 0x4c)
#define FC_DEBUG_TIMING0_REG	(FC_BASE + 0x50)
#define FC_DEBUG_TIMING1_REG	(FC_BASE + 0x54)

#define IO_READ_WORD(a)		mmio_read_32(a)
#define IO_WRITE_WORD(a, v) 	do {				\
					mmio_write_32(a, v);	\
				} while(0)

/**
 * sd_read_fuse: read fuse data from data-addr register
 * @param offset:	offset from fuse start
 * @param is_quad:	0 - one word to be read out; 1 - 4 words to be read out
 * @param out:		output buffer
 */
int sd_read_fuse(unsigned int offset, unsigned int is_quad, unsigned int *out)
{
	int addr_reg;
	int temp;
	// setup addr_reg for IO_READ_WORD command
	addr_reg = 0xC2000000 | // set interlock to 0xC2 to indicate the start of read operation
		(offset & 0x0000FFFF); // set fuse offset to addr
	// loop until IO_READ_WORD is successful
	do {
		// initiate IO_READ_WORD command
		do {
			// wait for IO_READ_WORD interface to be idle
			while ( (IO_READ_WORD(FC_READ_ADDR_REG)&0xFF000000) != 0x00000000);
			// attempt IO_READ_WORD, then check if command was accepted
			// i.e. busy, done or invalid
			IO_WRITE_WORD(FC_READ_ADDR_REG, addr_reg);
			temp = IO_READ_WORD(FC_READ_STATUS_REG);
			// exit if IO_READ_WORD had an error and return temp
			if (temp & 0x000000A0)
				return temp;
		}while (!(temp&0x11));  // loop again if busy or done bits are not active

		// wait for IO_READ_WORD to complete
		while (!(temp&0x00000010)) {
			temp = IO_READ_WORD(FC_READ_STATUS_REG);
		}
		if (temp & 0x000000A0)
			return temp;

		// copy word to data array
		out[0] = IO_READ_WORD(FC_READ_DATA0_REG);
		// copy three more words if performing 128-bit IO_READ_WORD
		if (is_quad)
		{
			out[1] = IO_READ_WORD(FC_READ_DATA1_REG);
			out[2] = IO_READ_WORD(FC_READ_DATA2_REG);
			out[3] = IO_READ_WORD(FC_READ_DATA3_REG);
		}
		// check done bit one more time
		// if it is still set then the correct data was IO_READ_WORD from the registers
		// if it is not set then another client¡¯s data may have overwritten
		// data in the registers before they were IO_READ_WORD
	}while(0==(IO_READ_WORD(FC_READ_STATUS_REG)&0x10));

	return 0;
}

#if 0
#define WRITE_UNLOCK_CODE_1 0x000C0DE1
#define WRITE_UNLOCK_CODE_2 0x000C0DE2
#define WRITE_UNLOCK_CODE_3 0x000C0DE3
#define WRITE_UNLOCK_CODE_4 0x000C0DE4

int sd_write_fuse(unsigned int addr, unsigned int words, unsigned int extra_unlock, unsigned char *data_array)
{
	int addr_reg;
	int data;
	int temp;
	// unlock the programming interface
	temp = 1;
	do {
		IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_1);
		IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_2);
		// optionally provide 4 unlock codes
		if (extra_unlock)
		{
			IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_3);
			IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_4);
		}
		// attempt to IO_WRITE_WORD data register to verify unlock was successful
		IO_WRITE_WORD(FC_WRITE_DATA0_REG, 0x01234567);
	}while(IO_READ_WORD(FC_WRITE_DATA0_REG) != 0x01234567);
	// setup addr_reg for IO_WRITE_WORD command
	addr_reg = 0xC2000000 | // set interlock to 0xC2
		    0x00800000 | // set bit 23 to program data
		    (addr & 0x0000FFFF); // set fuse offset to addr
	// set data pointer to data array
	data = data_array;
	// setup first data word in register
	IO_WRITE_WORD(FC_WRITE_DATA0_REG, data);
	// setup three more data words if performing 128-bit IO_WRITE_WORD
	if (words == 4) {
		IO_WRITE_WORD(FC_WRITE_DATA1_REG, ++data);
		IO_WRITE_WORD(FC_WRITE_DATA2_REG, ++data);
		IO_WRITE_WORD(FC_WRITE_DATA3_REG, ++data);
	}
	// wait for programming interface to be IO_READ_WORDy
	temp = 0xC2000000;
	while (temp != 0x00000000) {
		temp = IO_READ_WORD(FC_WRITE_ADDR_REG);
		temp &= 0xFF000000;
	}
	// execute IO_WRITE_WORD to fuse array
	IO_WRITE_WORD(FC_WRITE_ADDR_REG, addr_reg);
	// wait for IO_WRITE_WORDs to complete
	temp = 0;
	while (!temp) {
		temp = IO_READ_WORD(FC_WRITE_STATUS_REG);
		temp &= 0x00000010;
	}

	// check for errors
	temp = IO_READ_WORD(FC_WRITE_STATUS_REG);
	temp &= 0x000000A0;
	// re-lock the programming interface
	IO_WRITE_WORD(FC_WRITE_LOCK_REG, 0);
	// return temp,
	// 0, if programming was successful
	// non-zero, if there was an error
	return temp;
}
#endif

