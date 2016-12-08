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
#include <dcsn_sec.h>

DECLARE_DCSN_CTRL;

static void dcsn_set_access(volatile uint32_t *acc, uint32_t val)
{
	*acc = val;
}

#define DCSN_SEC_ARM_SEC	(DCSN_SEC_ARM_SR | DCSN_SEC_ARM_SW)

void sd_dcsn_set_protections(void)
{
	do_for_each_dcs_config(dcsn_set_access, DCSN_SEC_ARM_SEC);
	do_for_each_dcs_security(dcsn_set_access, DCSN_SEC_ARM_SEC);
	dcs_target_access_turing = DCSN_SEC_ARM_SEC | DCSN_SEC_DISPMIPS;
	dcs_target_access_pman_sec0 = DCSN_SEC_ARM_SEC;
#if PMAN_SEC_GROUP_MAX > 1
	dcs_target_access_pman_sec1 = DCSN_SEC_ARM_SEC;
#endif
#if PMAN_SEC_GROUP_MAX > 2
	dcs_target_access_pman_sec2 = DCSN_SEC_ARM_SEC;
#endif
	dcs_target_access_sec_timer = DCSN_SEC_ARM_SEC;
}

void sd_dcsn_drop_protections(void)
{
	do_for_each_dcs_config(dcsn_set_access, DCSN_SEC_EVERYBODY);
	do_for_each_dcs_security(dcsn_set_access, DCSN_SEC_EVERYBODY);
	dcs_target_access_turing = DCSN_SEC_EVERYBODY;
	dcs_target_access_pman_sec0 = DCSN_SEC_EVERYBODY;
#if PMAN_SEC_GROUP_MAX > 1
	dcs_target_access_pman_sec1 = DCSN_SEC_EVERYBODY;
#endif
#if PMAN_SEC_GROUP_MAX > 2
	dcs_target_access_pman_sec2 = DCSN_SEC_EVERYBODY;
#endif
	dcs_target_access_sec_timer = DCSN_SEC_EVERYBODY;
}

