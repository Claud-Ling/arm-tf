/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
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
#include <string.h>
#include <platform_def.h>
#include <sd_private.h>
#include <dcsn_sec.h>
#include <pman_sec.h>
#include <cpu/a53_cfg.h>

#define A53_CFG_REG	0xFB040010

/* define DCSN CTRL */
DEFINE_DCSN_CTRL(SD_PLF_DCSN_SEC_BASE,SD_AV_DCSN_SEC_BASE,SD_DISP_DCSN_SEC_BASE);

/* define PMAN_SEC */
DEFINE_PMAN_SEC_GROUPS((struct pman_sec*)SD_PMAN_SEC0_BASE);

/*
 * We assume that all security programming is done by the primary core.
 */
void sd_soc_set_protections(void)
{
	volatile union A53_CFGReg *cfg = (volatile union A53_CFGReg *)A53_CFG_REG;
	cfg->val |= ((1 << A53_CFG_iram_secure_SHIFT) |
		     (1 << A53_CFG_irom_secure_SHIFT));
}

/*
 * fn: int sd_soc_get_ddr_layout(ddr_block_t blobs[], int nb);
 * return number of umacs on success with address space of
 * each filled in array pointed by blobs, indexed by id.
 * Otherwise return error code (<0).
 */
int sd_soc_get_ddr_layout(ddr_block_t blobs[], const int nb)
{
	if (nb < CONFIG_SIGMA_NR_UMACS) {
		ERROR("small buffer!\n");
		return -1;
	}
	memset(blobs, 0, nb * sizeof(ddr_block_t));
	/* UMAC0 */
	blobs[0].start = 0x00000000;
	blobs[0].end   = 0x40000000;
	//TODO...
	return 1;
}
