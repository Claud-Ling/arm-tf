/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <bl_common.h>
#include <assert.h>
#include <mmio.h>
#include <debug.h>
#include <string.h>
#include <sd_private.h>

#define OTP_SYS_OFS_MASK 0x3ff
#define OTP_SYS_ADDR(ofs) 		\
	(SD_OTP_FUSE_BASE + ((ofs) & OTP_SYS_OFS_MASK))
#define FUSE_ADDR(ofs)	OTP_SYS_ADDR(0x100 + (ofs))

#define PMAN_SEC_OFS_MASK 0xfff
#define PMAN_SEC_ADDR(id, ofs)		\
	(SD_PMAN_SEC##id##_BASE + ((ofs) & (PMAN_SEC_OFS_MASK)))

#define DCSN_CFG_OFS_MASK 0xfff
#define DCSN_CFG_ADDR(nm, ofs)		\
	(SD_##nm##_DCSN_SEC_BASE + 0x1000 + ((ofs) & DCSN_CFG_OFS_MASK))

#define RACCESS_ENTRY(addr, amask, mask, op)	\
	{(addr), (amask), (mask), (op)},


#define PMAN_SEC_GROUP(id)										\
	RACCESS_ENTRY(PMAN_SEC_ADDR(id, 0x000), 0xfffff000, 0xffffffff, O_RD | O_WORD)	/*whole PMAN_SEC*/	\
	RACCESS_ENTRY(PMAN_SEC_ADDR(id, 0xfd8), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*INT_CLR_ENABLE*/	\
	RACCESS_ENTRY(PMAN_SEC_ADDR(id, 0xfdc), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*INT_SET_ENABLE*/	\
	RACCESS_ENTRY(PMAN_SEC_ADDR(id, 0xfe8), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*INT_CLR_STATUS*/	\
	RACCESS_ENTRY(PMAN_SEC_ADDR(id, 0xfec), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*INT_SET_STATUS*/


#define DCSN_CFG_GROUP(nm)										\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0x00c), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*bc_addr*/		\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0x010), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*bc_stat*/		\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0xfd8), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*bc_int_clr_en*/	\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0xfdc), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*bc_int_set_en*/	\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0xfe0), 0xffffffff, 0x00000003, O_RD | O_WORD)	/*bc_int_status*/	\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0xfe4), 0xffffffff, 0x00000003, O_RD | O_WR | O_WORD)	/*bc_int_en*/	\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0xfe8), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*bc_int_clr_sel*/	\
	RACCESS_ENTRY(DCSN_CFG_ADDR(nm, 0xfec), 0xffffffff, 0xffffffff, O_WR | O_WORD)	/*bt_int_set_sel*/

struct regaccess{
	uint32_t addr;	/*physical addr*/
	uint32_t amask; /*address mask*/
	uint32_t mask;	/*
			 * readable/writable mask
			 * hide non-readable bits on read
			 * reserve non-writable bits on write
			 */
#define MODE_MASK 0x3
#define O_BYTE	(0 << 0)
#define O_HWORD	(1 << 0)
#define O_WORD	(2 << 0)
#define ACCESS_MASK (0x3 << 2)
#define O_RD	(1 << 2)
#define O_WR	(1 << 3)
	uint32_t op;	/*
			 * [31:4] reserved
			 * [3] writable
			 * [2] readable
			 * [1:0] access mode
			 *	2'b00  - byte
			 *	2'b01  - half-word
			 *	2'b10  - word
			 *	2'b11  - N/A
			 */
} allowtbl[] = {
	/*OTP registers*/
	RACCESS_ENTRY(FUSE_ADDR(0x04), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*FC_0*/
	RACCESS_ENTRY(FUSE_ADDR(0x08), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*FC_1*/
	RACCESS_ENTRY(FUSE_ADDR(0x0c), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*FC_2, for BOOT_FROM_ROM, SEC_BOOT_EN & BOOT_VAL_USER_ID, etc*/
	RACCESS_ENTRY(FUSE_ADDR(0x10), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*FC_3, for SEC_DIS*/
	RACCESS_ENTRY(FUSE_ADDR(0x38), 0xffffffff, 0xffffffff, O_RD | O_WORD)	/*DIE_ID_0, for chip version*/

	/*PMAN SEC registers*/
	PMAN_SEC_GROUP(0)
#if SD_PMAN_NR_GRPS > 1
	PMAN_SEC_GROUP(1)
#endif
#if SD_PMAN_NR_GRPS > 2
	PMAN_SEC_GROUP(2)
#endif

	/*DCSN Configure registers*/
	DCSN_CFG_GROUP(PLF)
	DCSN_CFG_GROUP(AV)
	DCSN_CFG_GROUP(DISP)

	{-1, -1}		/*the end*/
};

/*
 * return 0 on access granted, non-zero otherwise.
 */
static int lookup_access_table(const uintptr_t addr, uint32_t op, struct regaccess *e)
{
	int i, ret = -1;
	for (i = 0; allowtbl[i].addr != -1; i++) {
		if (!((addr ^ allowtbl[i].addr) & allowtbl[i].amask) &&
			((op & MODE_MASK) == (allowtbl[i].op & MODE_MASK)) &&
			((op & ACCESS_MASK) & (allowtbl[i].op & ACCESS_MASK))) {
			memcpy(e, &allowtbl[i], sizeof(struct regaccess));
			VERBOSE("hit entry: %x %x %x %x\n", e->addr, e->amask, e->mask, e->op);
			ret = 0;
			break;
		}
	}
	return ret;
}

#define define_read_reg(magic)							\
static int read_reg_uint##magic(uintptr_t addr, unsigned long* const pval)	\
{										\
	int _ret = SD_SIP_E_PERMISSION_DENY;					\
	struct regaccess entry;							\
	if (lookup_access_table(addr, 						\
		O_RD | (((magic) >> 4) & MODE_MASK), &entry) == 0) {		\
		*(pval) = mmio_read_##magic(addr) & entry.mask;			\
		_ret = SD_SIP_E_SUCCESS;					\
	} else {								\
		WARN("read_uint%s failed: (%lx)\n", #magic, addr);		\
	}									\
	return _ret;								\
}

#define define_write_reg(magic)							\
static int write_reg_uint##magic(uintptr_t addr, unsigned long val,		\
				 unsigned long mask)				\
{										\
	int _ret = SD_SIP_E_PERMISSION_DENY;					\
	struct regaccess entry;							\
	if (lookup_access_table(addr, 						\
		O_WR | (((magic) >> 4) & MODE_MASK), &entry) == 0) {		\
		unsigned long __tmp = (val);					\
		unsigned long __mask = (mask) & entry.mask;			\
		if (__mask != ((1ull << (magic)) - 1)) {			\
			/*mask write*/						\
			__tmp = mmio_read_##magic(addr);			\
			__tmp = (__tmp & (~__mask)) | ((val) & __mask);		\
		}								\
		mmio_write_##magic(addr, __tmp); 				\
		_ret = SD_SIP_E_SUCCESS;					\
	} else {								\
		WARN("write_uint%s failed: (%lx)\n", #magic, addr);		\
	}									\
	return _ret;								\
}

define_read_reg(8)	/*byte*/
define_read_reg(16)	/*hword*/
define_read_reg(32)	/*word*/
define_write_reg(8)	/*byte*/
define_write_reg(16)	/*hword*/
define_write_reg(32)	/*word*/

/*
 * read one register
 * mode: specifies access mode
 *    	0 - byte
 *	1 - halfword
 *	2 - word
 * return value:
 * 	SD_SIP_E_SUCCESS on success, or error code otherwise.
 */
int sd_sip_mmio_read(const uint32_t mode, const paddr_t pa, unsigned long * const pout)
{
	int ret = SD_SIP_E_FAIL;
	void *va = NULL;
	size_t len = 1 << (mode & SEC_MMIO_MODE_MASK);

	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_IO, pa, len) ||
	    !ALIGNMENT_IS_OK(pa, len) ||
	    !(va = sd_phys_to_virt(pa))) {
		ERROR("%s: Bad address %lx\n", __func__, pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	assert(pout != NULL);
	if (SEC_MMIO_MODE_BYTE == mode) {
		ret = read_reg_uint8((uintptr_t)va, pout);
	} else if (SEC_MMIO_MODE_HWORD == mode) {
		ret = read_reg_uint16((uintptr_t)va, pout);
	} else if (SEC_MMIO_MODE_WORD == mode) {
		ret = read_reg_uint32((uintptr_t)va, pout);
	} else {
		WARN("read_reg failed: unknown mode code (%x)\n", mode);
		ret = SD_SIP_E_INVALID_PARAM;
	}
	return ret;
}

/*
 * write one register
 * mode: specifies access mode
 *    	0 - byte
 *	1 - halfword
 *	2 - word
 *
 * return value: 
 *	SD_SIP_E_SUCCESS on success, or error code otherwise.
 */
int sd_sip_mmio_write(const uint32_t mode, const paddr_t pa, const unsigned long val, const unsigned long mask)
{
	void *va = NULL;
	size_t len = 1 << (mode & SEC_MMIO_MODE_MASK);

	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_IO, pa, len) ||
	    !ALIGNMENT_IS_OK(pa, len) ||
	    !(va = sd_phys_to_virt(pa))) {
		ERROR("%s: Bad address %lx\n", __func__, pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	if (SEC_MMIO_MODE_BYTE == mode) {
		return write_reg_uint8((uintptr_t)va, val, mask);
	} else if (SEC_MMIO_MODE_HWORD == mode) {
		return write_reg_uint16((uintptr_t)va, val, mask);
	} else if (SEC_MMIO_MODE_WORD == mode) {
		return write_reg_uint32((uintptr_t)va, val, mask);
	} else {
		WARN("write_reg failed: unknown mode code (%x)\n", mode);
		return SD_SIP_E_INVALID_PARAM;
	}
}
