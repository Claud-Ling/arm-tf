#include <bl_common.h>
#include <debug.h>
#include <platform_def.h>
#include "plat_pinshare.h"

/*
 * In:
 *	id	- SDHCI id, 0:emmc, 1:SDIO
 */
int sd_soc_pinshare_init_for_mmc(int id)
{
	if (id == 0) {
		config_pin_share(PLAT_PIN_SHARE_FAD0, FAD0_MASK, FAD0_SDIO_DAT0);
		config_pin_share(PLAT_PIN_SHARE_FAD1, FAD1_MASK, FAD1_SDIO_DAT1);
		config_pin_share(PLAT_PIN_SHARE_FAD2, FAD2_MASK, FAD2_SDIO_DAT2);
		config_pin_share(PLAT_PIN_SHARE_FAD3, FAD3_MASK, FAD3_SDIO_DAT3);
		config_pin_share(PLAT_PIN_SHARE_FAD4, FAD4_MASK, FAD4_SDIO_DAT4);
		config_pin_share(PLAT_PIN_SHARE_FAD5, FAD5_MASK, FAD5_SDIO_DAT5);
		config_pin_share(PLAT_PIN_SHARE_FAD6, FAD6_MASK, FAD6_SDIO_DAT6);
		config_pin_share(PLAT_PIN_SHARE_FAD7, FAD7_MASK, FAD7_SDIO_DAT7);

		config_pin_share(PLAT_PIN_SHARE_CLE, CLE_MASK, CLE_SDIO_SDCLK);
		config_pin_share(PLAT_PIN_SHARE_ALE, ALE_MASK, ALE_SDIO_CMD);
		return 0;
	} else {
		WARN("not support id != 0 yet!\n");
		return 1;
	}
}

