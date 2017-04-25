
#include <bl_common.h>
#include <mmio.h>
#include <platform_def.h>
#include <sd_private.h>
#include <sd_otp.h>

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
 * @fn sd_read_fuse
 * @brief read fuse data and/or protections
 * @param[in]	offset	offset from fuse start
 * @param[in]	len	number of fuse words to be read, must be 1 or 4.
 * @param[out]	buf	buffer to load fuse data on return, set to NULL to ignore
 * @param[out]	pprot	buffer to load protection on return, set to NULL to ignore
 * @return	0 on success, or error code otherwise.
 */
int sd_read_fuse(const unsigned int offset, const int len, unsigned int *buf, unsigned int *pprot)
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
			// bit 9: ECC were zero
			// bit 8: internal error, read aborted
			// bit 7: data corrupted or blank ECC
			// bit 6: correctable data error
			// bit 5: invalid command
			// bit 4: done
			// bit 1-3: n/a
			// bit 0: busy
			if (temp & 0x000001A0)
				return temp;
		}while (!(temp&0x11));  // loop again if busy or done bits are not active

		// wait for IO_READ_WORD to complete
		while (!(temp&0x00000010)) {
			temp = IO_READ_WORD(FC_READ_STATUS_REG);
		}
		if (temp & 0x000001A0)
			return temp;

		if (buf != NULL) {
			// copy word to data array
			buf[0] = IO_READ_WORD(FC_READ_DATA0_REG);
			// copy three more words if performing 128-bit IO_READ_WORD
			if (len == 4)
			{
				buf[1] = IO_READ_WORD(FC_READ_DATA1_REG);
				buf[2] = IO_READ_WORD(FC_READ_DATA2_REG);
				buf[3] = IO_READ_WORD(FC_READ_DATA3_REG);
			}
		}

		if (pprot != NULL) {
			*pprot = IO_READ_WORD(FC_READ_PROT_REG);
		}
		// check done bit one more time
		// if it is still set then the correct data was IO_READ_WORD from the registers
		// if it is not set then another client¡¯s data may have overwritten
		// data in the registers before they were IO_READ_WORD
	}while(0==(IO_READ_WORD(FC_READ_STATUS_REG)&0x10));

	return 0;
}

#define WRITE_UNLOCK_CODE_1 0x000C0DE1
#define WRITE_UNLOCK_CODE_2 0x000C0DE2
#define WRITE_UNLOCK_CODE_3 0x000C0DE3
#define WRITE_UNLOCK_CODE_4 0x000C0DE4

/**
 * @fn sd_write_fuse
 * @brief write fuse data and/or protection attribute
 * @param[in]	offset	offset from fuse start
 * @param[in]	len	number of fuse words to be read, must be 1 or 4
 * @param[in]	buf	buffer loaded with fuse data on entry, or NULL
 * @param[int]	prot	protection attributes
 * @param[int]	flags	fuse write flags
 * @return	0 on success, or error code otherwise.
 */
int sd_write_fuse(const unsigned int offset, const int len, const unsigned int *buf, const unsigned int prot, const unsigned int flags)
{
	int addr_reg;
	int temp;
	if ((flags & (OTP_FLAG_DATA | OTP_FLAG_PROT)) == 0)
		return 0;
	if ((flags & OTP_FLAG_DATA) && (buf == NULL))
		return 1;
	if (len != 1 && len != 4)
		return 2;

	// unlock the programming interface
	do {
		IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_1);
		IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_2);
		// optionally provide 4 unlock codes
		if (flags & OTP_FLAG_UNLOCK_EXT)
		{
			IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_3);
			IO_WRITE_WORD(FC_WRITE_LOCK_REG, WRITE_UNLOCK_CODE_4);
		}
		// attempt to IO_WRITE_WORD data register to verify unlock was successful
		IO_WRITE_WORD(FC_WRITE_DATA0_REG, 0x01234567);
	}while(IO_READ_WORD(FC_WRITE_DATA0_REG) != 0x01234567);

	// setup addr_reg for IO_WRITE_WORD command
	addr_reg = 0xC2000000 | // set interlock to 0xC2
		    (offset & 0x0000FFFF); // set fuse offset to addr

	if ((flags & OTP_FLAG_DATA) && buf != NULL) {
		// set bit 23 to program data
		addr_reg |= 1<<23;
		// setup first data word in register
		IO_WRITE_WORD(FC_WRITE_DATA0_REG, buf[0]);
		// setup three more data words if performing 128-bit IO_WRITE_WORD
		if (len == 4) {
			IO_WRITE_WORD(FC_WRITE_DATA1_REG, buf[1]);
			IO_WRITE_WORD(FC_WRITE_DATA2_REG, buf[2]);
			IO_WRITE_WORD(FC_WRITE_DATA3_REG, buf[3]);
		}
	}

	if (flags & OTP_FLAG_PROT) {
		// set bit 22 to program protection
		addr_reg |= 1<<22;
		// Protect attribute
		IO_WRITE_WORD(FC_WRITE_PROT_REG, prot);
	}

	// wait for programming interface to be ready
	while ((IO_READ_WORD(FC_WRITE_ADDR_REG)&0xFF000000) != 0x00000000);

	// execute IO_WRITE_WORD to fuse array
	IO_WRITE_WORD(FC_WRITE_ADDR_REG, addr_reg);
	// wait for IO_WRITE_WORDs to complete
	while (!((temp=IO_READ_WORD(FC_WRITE_STATUS_REG)) & 0x00000010));

	// check for errors
	// 0, if programming was successful
	// non-zero, if there was an error
	// bit 8: Unexpected error, program aborted;
	// bit 7: internal programming error, programing failed;
	// bit 6: n/a
	// bit 5: invalid command supplied
	// bit 4: done
	// bit 2-3: n/a
	// bit [1:0]: number of write commands in FIFO
	temp = IO_READ_WORD(FC_WRITE_STATUS_REG);
	temp &= 0x000001A0;

	// re-lock the programming interface
	IO_WRITE_WORD(FC_WRITE_LOCK_REG, 0);
	// return value
	// 0, if programming was successful
	// non-zero, if there was an error
	return temp;
}

