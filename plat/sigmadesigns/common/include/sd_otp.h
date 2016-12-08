/******************************************
 * Copyright 2016
 * Sigma Designs, Inc. All Rights Reserved
 * Proprietary and Confidential
 ******************************************/

/**
 * @file	sd_otp.h
 * @brief	it declares fuse interfaces for Sigma Designs DTV SoCs
 *
 * examples:
 *   1.read BOOT_FROM_ROM fuse bit
 *     fuse_read_field(boot_from_rom); or fuse_field_boot_from_rom();
 *   2.read BOOT_ROM_CFG_1 value (flash_auth_length)
 *     fuse_read_entry(boot_rom_cfg_1); or OTP_FUSE_ENTRY(boot_rom_cfg_1);
 *   3.read rsa public key
 *     fuse_read_data(rsa_pub_key,buf,sz); or fuse_read_data_rsa_pub_key(buf,sz);
 *
 * @author:  Tony He <tony_he@sigmadesigns.com>
 * @date:    2016/11/28
 * 
 */

#include <sys/types.h>
#include <assert.h>
#include <fuse_data_map.h>

#ifndef __SD_OTP_H__
#define __SD_OTP_H__

#ifndef __ASSEMBLY__

#define DEFINE_FUSE_MAP(base)	\
struct fuse_data_map *fuse_map = (struct fuse_data_map*)(base)
#define DECLARE_FUSE_MAP	\
extern struct fuse_data_map *fuse_map

DECLARE_FUSE_MAP;

/*
 * @brief	read field value of specified register
 * @param[in]	nm	fuse register name
 * @param[in]	fn	field name
 * @return	return field value on success
 */
#define OTP_FUSE_FIELD(nm,fn)	fuse_map-> nm.bits.fn

/*
 * @brief	read whole value of specified fuse register
 * @param[in]	nm	fuse register name
 * @return	return register value on success
 */
#define OTP_FUSE_ENTRY(nm)	fuse_map-> nm

/*
 * otp data reading function helper
 */
#define OTP_FUSE_DATA(nm) 							\
static inline int fuse_read_data_##nm(uintptr_t buf, size_t sz)			\
{										\
	int i, ret = 1;								\
	assert(sz >= sizeof(fuse_map-> nm ));					\
	for (i=0; i<sizeof(fuse_map-> nm ); i+=16) {				\
		unsigned int ofs = (uintptr_t)(&fuse_map-> nm ) + i;		\
		ret = sd_read_fuse(ofs,	1, (unsigned int*)(buf+i));		\
		if (ret != 0)							\
			break;							\
	}									\
	return ret;								\
}

/**
 * @fn sd_read_fuse
 * @brief read fuse data from data-addr register
 * @param[in] offset	offset from fuse start
 * @param[in] is_quad	0 - one word to be read out; 1 - 4 words to be read out
 * @param out:		output buffer
 */
int sd_read_fuse(unsigned int offset, unsigned int is_quad, unsigned int *out);


/**
 * @fn	otp_read_rsa_pub_key(uintptr_t buf, size_t sz);
 * @brief	read rsa_pub_key from otp
 * @param[in]	buf	buffer pointer
 * @param[in]	sz	size of buffer, shall be no less than fuse array size
 * @return	0 on success
 */
OTP_FUSE_DATA(rsa_pub_key)

/*
 * otp read fuse field helpers
 */
#define FIELD_HELPER(reg, bf)			\
static inline int fuse_field_##bf (void)	\
{						\
	return OTP_FUSE_FIELD(reg, bf);		\
}

#include <fuse_helpers.h>
#undef FIELD_HELPER

/*
 * variants for user to choose
 */
/* read generic fuse array #nm */
#define fuse_read_data(nm, buf, sz) fuse_read_data_##nm(buf, sz)
/* read 4-bytes fuse entry #nm */
#define fuse_read_entry(nm)	OTP_FUSE_ENTRY(nm)
/* read fuse field #fn */
#define fuse_read_field(fn)	fuse_field_##fn()

#endif /*!__ASSEMBLY__*/

#endif /*__SD_OTP_H__*/
