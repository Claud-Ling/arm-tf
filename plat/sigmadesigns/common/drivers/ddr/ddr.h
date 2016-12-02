#ifndef __PREBOOT_INC_DDR_UTILS_H__
#define __PREBOOT_INC_DDR_UTILS_H__

void run_ddrauto(void);
void ddr_adjust(void);

/**
 * @brief	do dqs gating patch on specified umac in case it's activated.
 * @fn		int ddr_do_dqs_calibrate(int uid);
 * @param[in]	uid	umac id (0,1,...)
 * @retval	0	ok
 * @retval	1	fail
 */
int ddr_do_dqs_gating(int uid);

int RegTableSetup(void);

int RegTableWriteGroup(unsigned int groupId);

#endif /*__PREBOOT_INC_DDR_UTILS_H__*/
