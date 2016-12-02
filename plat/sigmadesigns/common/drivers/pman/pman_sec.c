/******************************************
 * Copyright 2016
 * Sigma Designs, Inc. All Rights Reserved
 * Proprietary and Confidential
 *
 * Author:  Tony He
 * Date:    2016/11/29
 *
 ******************************************/

#include <bl_common.h>
#include <platform_def.h>
#include <assert.h>
#include <pman_sec.h>

DECLARE_PMAN_SEC_GROUPS;

static int pman_get_group(unsigned long start, size_t sz)
{
	/*
	 * secure DRAM. Assume it's on umac0
	 * TODO: identify SEC DRAM location
	 */
	assert(start >= 0 && (start + sz) < 0x40000000);
	return 0;
}

void sd_pman_set_protections(void)
{
	uint32_t sec = PMAN_RGN_SEC_ARM_SEC | PMAN_RGN_SEC_TURING | PMAN_RGN_SEC_CRYPTO;
	uint32_t attr = PMAN_RGN_ATTR_WRITE | PMAN_RGN_ATTR_EXEC | PMAN_RGN_ATTR_READ | PMAN_RGN_ATTR_VALID;

	pman_sec_region_set(pman_get_group(SD_SEC_DRAM_BASE, SD_SEC_DRAM_SIZE), 0,
			    SD_SEC_DRAM_BASE, SD_SEC_DRAM_BASE + SD_SEC_DRAM_SIZE - 1,
			    sec, attr);
}

void sd_pman_drop_protections(void)
{
}


