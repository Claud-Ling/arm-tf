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
	dcs_target_access(turing) = DCSN_SEC_ARM_SEC | DCSN_SEC_DISPMIPS;
	dcs_target_access(pman_sec0) = DCSN_SEC_ARM_SEC;
#ifdef DCSN_TARGET_PMAN_SEC1
	dcs_target_access(pman_sec1) = DCSN_SEC_ARM_SEC;
#endif
#ifdef DCSN_TARGET_PMAN_SEC2
	dcs_target_access(pman_sec2) = DCSN_SEC_ARM_SEC;
#endif
	dcs_target_access(sec_timer) = DCSN_SEC_ARM_SEC;
}

void sd_dcsn_drop_protections(void)
{
	do_for_each_dcs_config(dcsn_set_access, DCSN_SEC_EVERYBODY);
	do_for_each_dcs_security(dcsn_set_access, DCSN_SEC_EVERYBODY);
	dcs_target_access(turing) = DCSN_SEC_EVERYBODY;
	dcs_target_access(pman_sec0) = DCSN_SEC_EVERYBODY;
#ifdef DCSN_TARGET_PMAN_SEC1
	dcs_target_access(pman_sec1) = DCSN_SEC_EVERYBODY;
#endif
#ifdef DCSN_TARGET_PMAN_SEC2
	dcs_target_access(pman_sec2) = DCSN_SEC_EVERYBODY;
#endif
	dcs_target_access(sec_timer) = DCSN_SEC_EVERYBODY;
}

