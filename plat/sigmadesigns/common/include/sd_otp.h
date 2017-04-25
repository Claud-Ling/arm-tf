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
#include <debug.h>	//DEBUG
#include <fuse_data_map.h>

#ifndef __SD_OTP_H__
#define __SD_OTP_H__

#ifndef __ASSEMBLY__

#define DEFINE_FUSE_MAP(base)	\
struct fuse_data_map *fuse_map = (struct fuse_data_map*)(base)
#define DECLARE_FUSE_MAP	\
extern struct fuse_data_map *fuse_map

DECLARE_FUSE_MAP;

#define OTP_WORD_SINGLE	1
#define OTP_WORD_QUAD	4

/**
 * @fn sd_read_fuse
 * @brief read fuse data and/or protections
 * @param[in]	offset	offset from fuse start
 * @param[in]	len	number of fuse words to be read, must be 1 or 4.
 * @param[out]	buf	buffer to load fuse data on return, set to NULL to ignore
 * @param[out]	pprot	buffer to load protection on return, set to NULL to ignore
 * @return	0 on success, or error code otherwise.
 */
int sd_read_fuse(const unsigned int offset, const int len, unsigned int *buf, unsigned int *pprot);

/*
 * fuse write flags
 */
enum {
	OTP_FLAG_DATA		= (1 << 0),	/*write fuse data*/
	OTP_FLAG_PROT		= (1 << 1),	/*write fuse protections*/
	OTP_FLAG_UNLOCK_EXT	= (1 << 2),	/*extra unlock*/
};
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
int sd_write_fuse(const unsigned int offset, const int len, const unsigned int *buf, const unsigned int prot, const unsigned int flags);

/**
 * @fn _fuse_read_data
 * @brief read fuse data and/or protections
 * @param[in]	ofs	offset from fuse start
 * @param[out]	buf	buffer to load fuse data on return, set to NULL to ignore
 * @param[in]	sz	number of bytes to read, must be multiple of 16
 * @param[out]	pprot	buffer to load protection on return, set to NULL to ignore
 * @return	0 on success, or error code otherwise.
 */
static inline int _fuse_read_data(const uint32_t ofs, const uintptr_t buf, const size_t sz, uint32_t*pprot)
{
	int i, ret = 1;
	unsigned int *tb = NULL;
	assert(sz > 4 && (sz & 0xf) == 0);
	for (i=0; i<sz; i+=16) {
		if (buf != 0) tb = (unsigned int*)(buf + i);
		ret = sd_read_fuse(ofs + i, OTP_WORD_QUAD, tb, pprot);
		if (ret != 0 || tb == NULL)
			break;
	}
	return ret;
}

/**
 * @fn _fuse_write_data
 * @brief write fuse data and/or protections
 * @param[in]	ofs	offset from fuse start
 * @param[in]	buf	buffer loaded with fuse data on entry, or NULL to ignore
 * @param[in]	sz	number of bytes to write, must be multiple of 16
 * @param[in]	prot	protection attributes
 * @return	0 on success, or error code otherwise.
 */
static inline int _fuse_write_data(const uint32_t ofs, const uintptr_t buf, const size_t sz, const uint32_t prot)
{
	int i, ret = 1;
	unsigned int *tb = NULL;
	uint32_t flags, curprot;
	assert(sz > 4 && (sz & 0xf) == 0);
	flags = (buf != 0) ? OTP_FLAG_DATA : 0;
	for (i=0; i<sz; i+=16) {
		curprot = 0;
		ret = sd_read_fuse(ofs + i, OTP_WORD_QUAD, NULL, &curprot);
		if (ret != 0)
			break;
		if (curprot != prot)
			flags |= OTP_FLAG_PROT;
		else
			flags &= ~OTP_FLAG_PROT;
		if (buf != 0) tb = (unsigned int*)(buf + i);
		ret = sd_write_fuse(ofs + i, OTP_WORD_QUAD, tb, prot, flags);
		if (ret != 0)
			break;
	}
	return ret;
}

/**
 * @fn fuse_size
 * @brief get fuse (data) size in bytes
 * @param[in]	ofs	offset from fuse start
 * @return	number of bytes on success, or error code otherwise.
 */
static inline int fuse_size(const uint32_t ofs)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(fuse_datas); i++) {
		if (ofs == fuse_datas[i].ofs)
			return fuse_datas[i].len;
	}
	return 4;
}

/*
 * size of fuse (data) of specified nm
 */
#define OTP_FUSE_SIZE(nm)	sizeof(fuse_map-> nm)

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
 * otp data reading/writing function helpers
 */
#define OTP_FUSE_DATA(nm) 							\
static inline int fuse_read_data_##nm(uintptr_t buf, size_t sz, uint32_t*pprot)	\
{										\
	assert(sz >= sizeof(fuse_map-> nm ));					\
	return _fuse_read_data(offsetof(struct fuse_data_map, nm), buf,		\
				sizeof(fuse_map-> nm), pprot);			\
}										\
										\
static inline int fuse_write_data_##nm(uintptr_t buf, size_t sz, uint32_t prot)	\
{										\
	assert(sz >= sizeof(fuse_map-> nm));					\
	return _fuse_write_data(offsetof(struct fuse_data_map, nm), buf,	\
				sizeof(fuse_map-> nm), prot);			\
}

/**
 * define fuse data helpers
 */
OTP_FUSE_DATA(otp_key_0)
OTP_FUSE_DATA(otp_key_1)
OTP_FUSE_DATA(otp_key_2)
OTP_FUSE_DATA(otp_key_3)
OTP_FUSE_DATA(otp_key_4)
OTP_FUSE_DATA(otp_key_5)
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
#define fuse_read_data(nm, buf, sz, pprot) fuse_read_data_##nm(buf, sz, pprot)
/* read 4-bytes fuse entry #nm */
#define fuse_read_entry(nm)	OTP_FUSE_ENTRY(nm)
/* read fuse field #fn */
#define fuse_read_field(fn)	fuse_field_##fn()

/*
 * otp access control
 */
typedef struct otp_access_ctrl {
	uint32_t ofs;	/*offset*/
	uint32_t mask;	/*bitwise mask*/
}otp_access_ctrl_t;

/*
 * @fn	sd_soc_otp_get_ns_list
 * @brief	fetch ns accessible otp list
 * @return	return list on success
 */
otp_access_ctrl_t * sd_soc_otp_get_ns_list(void);

/**
 * @fn sd_is_board_secured
 * @brief check if board is secured or not
 * @return	1 - secured, 0 - non-secured.
 *		It returns 1 always if env SD_SECURE is set to one.
 *		Otherwise return sec_boot_en fuse bit state.
 */
static inline int sd_is_board_secured(void)
{
#if SD_SECURE
	return 1;	/* force on */
#else
	return !!fuse_read_field(sec_boot_en);
#endif
}

#endif /*!__ASSEMBLY__*/

#endif /*__SD_OTP_H__*/
