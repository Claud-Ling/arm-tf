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
#include <assert.h>
#include <console.h>
#include <debug.h>
#include <mmio.h>
#include <sd_sip_svc.h>
#include <plat_sip_calls.h>
#include <runtime_svc.h>
#include <uuid.h>
#include <sd_private.h>
#include "sd_sip_private.h"

/*
 * SigmaDesigns SiP Service UUID:
 * 82af283f-67d7-49bb-8323-002a5d926486
 */
DEFINE_SVC_UUID(sd_sip_svc_uid,
		0x82af283f, 0x67d7, 0x49bb, 0x83, 0x23,
		0x00, 0x2a, 0x5d, 0x92, 0x64, 0x86);

#pragma weak sd_plat_sip_handler
uint64_t sd_plat_sip_handler(uint32_t smc_fid,
				uint64_t x1,
				uint64_t x2,
				uint64_t x3,
				uint64_t x4,
				void *cookie,
				void *handle,
				uint64_t flags)
{
	ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
	SMC_RET1(handle, SMC_UNK);
}

/*
 * This function handles Sigma Designs defined SiP Calls
 */
static uint64_t sd_sip_handler(uint32_t smc_fid,
			uint64_t x1,
			uint64_t x2,
			uint64_t x3,
			uint64_t x4,
			void *cookie,
			void *handle,
			uint64_t flags)
{
	uint64_t ret;
	uint32_t ns;
	uint32_t tmp = 0, len;

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);

	/* Common services */
	switch (smc_fid) {
	case SD_SIP_FUNC_C_MEM_STATE:
		ret = sd_sip_get_access_state(reg_pair_to_paddr(x1, x2), x3, &tmp);
		SMC_RET2(handle, ret, tmp);
	case SD_SIP_FUNC_C_OTP_READ:
		len = (uint32_t)x4;
		ret = sd_sip_otp_read(x1, reg_pair_to_paddr(x2, x3), &len, &tmp, ns);
		SMC_RET3(handle, ret, tmp, len);
	}

	/* Secure awareness services */
	if (!ns) {
		/* SiP SMC service secure world's call */
		switch (smc_fid) {
			case SD_SIP_FUNC_S_OTP_WRITE:
				len = (uint32_t)x4;
				ret = sd_sip_otp_write(x1, reg_pair_to_paddr(x2, x3), &len,
					       read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5));
				SMC_RET2(handle, ret, len);
		}
	} else {
		/* SiP SMC service normal world's call */
		switch (smc_fid) {
		case SD_SIP_FUNC_N_SET_PST:
			ret = sd_sip_set_pst(reg_pair_to_paddr(x1, x2), x3);
			SMC_RET1(handle, ret);
		case SD_SIP_FUNC_N_MMIO:
			if (x1 & SEC_MMIO_CMD_WNR) {
				ret = sd_sip_mmio_write(SEC_MMIO_MODE(x1), reg_pair_to_paddr(x2, x3), x4,
						        read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5));
				SMC_RET1(handle, ret);
			} else {
				unsigned long tmpl;
				ret = sd_sip_mmio_read(SEC_MMIO_MODE(x1), reg_pair_to_paddr(x2, x3), &tmpl);
				SMC_RET2(handle, ret, tmpl);
			}
		case SD_SIP_FUNC_N_RSA_KEY:
			ret = sd_sip_get_rsa_pub_key(reg_pair_to_paddr(x1, x2), x3);
			SMC_RET1(handle, ret);
		}
	}

	/* Platform specific services */
	return sd_plat_sip_handler(smc_fid, x1, x2, x3, x4,
					cookie, handle, flags);
}

/*
 * This function is responsible for handling all SiP calls from the S and NS world
 */
uint64_t sip_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags)
{
	VERBOSE("==> SMC call fid %x flags %lx, args: %lx %lx\n", smc_fid, flags, x1, x2);
	switch (smc_fid) {
	case SIP_SVC_CALL_COUNT:
		/* Return the number of Sigma Designs SiP Service Calls. */
		SMC_RET1(handle,
			 SD_COMMON_SIP_NUM_CALLS + SD_PLAT_SIP_NUM_CALLS);

	case SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, sd_sip_svc_uid);

	case SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, SD_SIP_SVC_VERSION_MAJOR,
			SD_SIP_SVC_VERSION_MINOR);

	default:
		return sd_sip_handler(smc_fid, x1, x2, x3, x4,
			cookie, handle, flags);
	}
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	sd_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	NULL,
	sip_smc_handler
);
