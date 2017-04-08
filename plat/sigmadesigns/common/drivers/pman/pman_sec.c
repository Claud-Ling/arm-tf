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
#include <sd_private.h>
#include "pman_sec_private.h"

void sd_pman_set_protections(void)
{
	int ret;
	ret = pman_set_protections();
	assert((pman_get_access_state(SD_SEC_DRAM_BASE, SD_SEC_DRAM_SIZE) & MEM_STATE_MASK)
		 == (MEM_STATE_S_RW | MEM_STATE_S_EXEC));
	assert(ret == PMAN_OK);
	(void)ret;
}

void sd_pman_drop_protections(void)
{
#ifndef WITH_PROD
	pman_drop_protections();
#endif
}

int sd_pman_update_protections(const uint32_t tpa, const uint32_t sz)
{
#ifndef WITH_PROD
	return pman_update_protections(tpa, sz);
#else
	return PMAN_NOT_SUPPORT;
#endif
}

int sd_pman_get_access_state(const uint32_t pa, const uint32_t sz)
{
	return pman_get_access_state(pa, sz);
}
