#include <bl_common.h>
#include <platform_def.h>
#include <debug.h>
#include <sd_private.h>
#include "ddr.h"

#define GROUP_TUNING	1	/*DDR settings part2*/ 
#define GROUP_BALANCE	2	/*settings to start with*/
#define GROUP_TUNING1	6	/*DDR settings part1*/

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 *****************************************************************************/
#pragma weak sd_early_boot_mode
#pragma weak sd_umac_access_mode
#pragma weak sd_umac_set_tr_area

/*
 * TODO: get boot mode
 * @return	0 - normal mode
 *		1 - resume from standby
 */
int sd_early_boot_mode(void)
{
	return SD_BOOT_NORMAL;
}

/*
 * TODO: put all umac in access mode
 */
void sd_umac_access_mode(void)
{
}

/*
 * TODO: set training buffer for umacs
 */
void sd_umac_set_tr_area(void)
{
}

int sd_ddr_init(void)
{
	RegTableSetup();
	RegTableWriteGroup(GROUP_BALANCE);
	/*download TUNING1 table(before do_s2ram)*/
	RegTableWriteGroup(GROUP_TUNING1);

	/*
	 * Recovery umac to access mode
	 * It is only needed in case of resume
	 */
	if (sd_early_boot_mode() == SD_BOOT_RESUME) {
		sd_umac_access_mode();
		sd_umac_set_tr_area();
	}

	RegTableWriteGroup(GROUP_TUNING);
#ifndef CONFIG_SKIP_DDR_ADJUST
	ddr_adjust();
#endif
	//TODO:auto scan...

	return 0;
}
