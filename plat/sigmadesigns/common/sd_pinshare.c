#include <bl_common.h>
#include <debug.h>

/*
 * The following platform setup functions are weakly defined. They
 * provide typical implementations that will be overridden by a SoC.
 */
#pragma weak sd_soc_pinshare_init_for_mmc

int sd_soc_pinshare_init_for_mmc(int id)
{
	return 0;
}
