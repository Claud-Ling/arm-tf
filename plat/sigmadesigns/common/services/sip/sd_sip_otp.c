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
#include <sd_otp.h>

#define otp_to_sip_ret(rc) (((rc) == 0) ? SD_SIP_E_SUCCESS : SD_SIP_E_FAIL)

/*
 * request to read otp from secure os
 */
static int sec_read_otp(const size_t ofs, const paddr_t pa, const size_t len, uint32_t *const pprot)
{
	int ret;
	uint32_t flen, *va = NULL;
	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_SEC, pa, len) ||
	    !ALIGNMENT_IS_OK(pa, uint32_t) ||
	    (!(va = (uint32_t*)sd_phys_to_virt(pa)) && (pa != 0))) {
		ERROR("Bad address %lx\n", pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	/* check buffer length */
	flen = fuse_size(ofs);
	if (len < flen) {
		ERROR("Small buffer for otp data 0x%zx\n", ofs);
		return SD_SIP_E_SMALL_BUFFER;
	}

	if (flen > 4)
		ret = _fuse_read_data(ofs, (uintptr_t)va, flen, pprot);
	else
		ret = sd_read_fuse(ofs, OTP_WORD_SINGLE, (unsigned int*)va, pprot);

	return otp_to_sip_ret(ret);
}

/*
 * request to read otp from non-secure world
 */
static int ns_read_otp(const size_t ofs, const paddr_t pa, const size_t len, uint32_t *const pprot)
{
	int ret;
	uint32_t tmp, *va = NULL;
	otp_access_ctrl_t *ns_lst;

	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_NS, pa, len) ||
	    !ALIGNMENT_IS_OK(pa, uint32_t) ||
	    (!(va = (uint32_t*)sd_phys_to_virt(pa)) && (pa != 0))) {
		ERROR("Bad address %lx\n", pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	/* if it's accessible to ns world */
	ns_lst = sd_soc_otp_get_ns_list();
	while(ns_lst->ofs != -1) {
		if (ofs == ns_lst->ofs) {
			VERBOSE("find ofs %x in ns otp list\n", ofs);
			break;
		}
		ns_lst++;
	}

	if (ns_lst->ofs != -1) {
		/* check buffer length */
		tmp = fuse_size(ofs);
		if (len < tmp) {
			ERROR("Small buffer for otp 0x%zx\n", ofs);
			return SD_SIP_E_SMALL_BUFFER;
		}

		switch (ofs) {
		case FUSE_OFS_FC_0:
		case FUSE_OFS_FC_1:
		case FUSE_OFS_FC_2:
		case FUSE_OFS_FC_3:
		case FUSE_OFS_DIE_ID_0:
			ret = sd_read_fuse(ofs, OTP_WORD_SINGLE, &tmp, pprot);
			if ((0 == ret)&& (va != NULL)) {
				*va = tmp & ns_lst->mask;
			}
			ret = otp_to_sip_ret(ret);
			break;
		case FUSE_OFS_RSA_PUB_KEY:
			ret = fuse_read_data(rsa_pub_key, (uintptr_t)va, len, pprot);
			ret = otp_to_sip_ret(ret);
			break;
		default:
			WARN("otp passes access check but unknown here\n");
			ret = SD_SIP_E_FAIL;
			break;
		}
	} else {
		ret = SD_SIP_E_PERMISSION_DENY;
	}
	return ret;
}

/*
 * secure and normal accessible
 * pa != 0   - read fuse data & protection
 * pa == 0   - read protection
 */
int sd_sip_otp_read(const size_t ofs, const paddr_t pa, const size_t len, uint32_t *const pprot, const uint32_t ns)
{
	if (ns)
		return ns_read_otp(ofs, pa, len, pprot);
	else
		return sec_read_otp(ofs, pa, len, pprot);
}

/*
 * secure accessible only
 * pa != 0    - write fuse data & protection
 * pa == 0    - write protection only
 */
int sd_sip_otp_write(const size_t ofs, const paddr_t pa, const size_t len, const uint32_t prot)
{
	int ret;
	uint32_t flen, *va = NULL;
	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_SEC, pa, len) ||
	    !ALIGNMENT_IS_OK(pa, uint32_t) ||
	    (!(va = (uint32_t*)sd_phys_to_virt(pa)) && (pa != 0))) {
		ERROR("Bad address %lx\n", pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	/* check buffer length */
	flen = fuse_size(ofs);
	if (len < flen) {
		ERROR("Small buffer for otp data 0x%zx\n", ofs);
		return SD_SIP_E_SMALL_BUFFER;
	}

	if (flen > 4) {
		ret = _fuse_write_data(ofs, (uintptr_t)va, flen, prot);
	} else {
		uint32_t flags, curprot = 0;
		flags = (va != NULL) ? OTP_FLAG_DATA : 0;
		if ((ret = sd_read_fuse(ofs, OTP_WORD_SINGLE, NULL, &curprot)) != 0) {
			INFO("fail to read prot for otp %zx\n", ofs);
			goto OUT;
		}
		if (curprot != prot)
			flags |= OTP_FLAG_PROT;
		else
			flags &= ~OTP_FLAG_PROT;
		ret = sd_write_fuse(ofs, OTP_WORD_SINGLE, (unsigned int*)va, prot, flags);
	}

OUT:
	return otp_to_sip_ret(ret);
}

/*
 * normal accessible only
 * get current using rsa public key
 * boot_val_user_id:   0 - load from otp
 *                     1 - load from ROM
 */
int sd_sip_get_rsa_pub_key(const paddr_t pa, const size_t len)
{
	int ret, key_idx = 0;
	uint32_t *va = NULL;
	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_NS, pa, len) ||
	    !ALIGNMENT_IS_OK(pa, uint32_t) ||
	    (!(va = (uint32_t*)sd_phys_to_virt(pa)) && (pa != 0))) {
		ERROR("Bad address %lx\n", pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	/* check buffer length */
	if (len < 256) {
		ERROR("Small buffer for rsa pub key\n");
		return SD_SIP_E_SMALL_BUFFER;
	}

	key_idx = fuse_read_field(boot_val_user_id);
	if (0 == key_idx) {
		/* use key from otp */
		ret = fuse_read_data(rsa_pub_key, (uintptr_t)va, len, NULL);
		ret = otp_to_sip_ret(ret);
	} else {
		/* use key from ROM */
		uintptr_t kpos;
		key_idx--;
		kpos = SD_ROM_KEY_BASE + (key_idx << 8);
		memcpy(va, (void*)kpos, 256);
		ret = SD_SIP_E_SUCCESS;
	}
	return ret;
}
