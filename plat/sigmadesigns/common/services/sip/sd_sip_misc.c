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
#include <sd_private.h>
#include <sd_otp.h>

int sd_sip_set_pst(const paddr_t pa, const size_t len)
{
	int ret;
	void *va = NULL;
	/* sanity check the input address */
	if (!sd_pbuf_is(MEM_NS, pa, len) ||
	    !ALIGNMENT_IS_TYPE(pa, uint32_t) ||
	    !(va = sd_phys_to_virt(pa))) {
		ERROR("%s: Bad address %lx\n", __func__, pa);
		return SD_SIP_E_INVALID_RANGE;
	}

	if (!sd_is_board_secured()) {
		/* service only in case of non-secured boards */
		ret = sd_pman_update_protections((uintptr_t)va, len);
		if (PMAN_E_OK == ret)
			ret = SD_SIP_E_SUCCESS;
		else if (PMAN_E_INVAL == ret)
			ret = SD_SIP_E_INVALID_PARAM;
		else if (PMAN_E_NOT_SUPPORT == ret)
			ret = SD_SIP_E_NOT_SUPPORTED;
		else
			ret = SD_SIP_E_FAIL;
	} else {
		ret = SD_SIP_E_NOT_SUPPORTED;
	}
	return ret;
}

int sd_sip_get_access_state(const paddr_t pa, const size_t len, uint32_t * const pout)
{
	int ret;
	ret = sd_pman_get_access_state(pa, len);
	if (ret != 0) {
		*pout = ret;
		return SD_SIP_E_SUCCESS;
	} else {
		return SD_SIP_E_INVALID_PARAM;
	}
}
