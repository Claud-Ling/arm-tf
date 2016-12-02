
#include <assert.h>
#include <platform_def.h>
#include <sd_private.h>
#include "umac.h"

int ddr_do_dqs_gating(int uid)
{
	if (umac_is_activated(uid)) {
		struct umac_phy* phy = umac_get_phy(uid);
		const unsigned int nl = UMAC_NR_OF_LANES(uid);
		unsigned int rslr[4], minrslr;
		int i, need_patch = 0;
		assert(phy != NULL);
		/*Get all rslrX and compare them.*/
		rslr[0] = phy->pub.drlcfg_0.bits.rslr0;
		rslr[1] = phy->pub.drlcfg_0.bits.rslr1;
		rslr[2] = phy->pub.drlcfg_0.bits.rslr2;
		rslr[3] = phy->pub.drlcfg_0.bits.rslr3;
		minrslr = rslr[0];
		for (i = 1; i < nl; i++) {
			if (minrslr != rslr[i]) {
				need_patch = 1;
				if (minrslr > rslr[i])
					minrslr = rslr[i];
			}
		}

		if (need_patch) {
			/*update DQSDGX*/
#ifdef CONFIG_UMAC_DQS_NEW_METHOD
			unsigned int val = phy->pub.ctl_dqsecfg & (~0xffff); /*clear alat bytes*/
			for (i = 0; i < nl; i++)
				val |= (rslr[i] << (4*i));
			phy->pub.ctl_dqsecfg = val;
			phy->pub.ctl_dqsecfg_pe |= 1;	/*set ctl_dqsecfg_parallel_en*/
#else
			U32 val = phy->pub.pub_tr_rd_lcdl & 0xff; /*get lcdl_shift*/
			for (i = 0; i < nl; i++) {
				volatile U32 *p = &phy->pub.phy_dx0lcdlr2 +
						((i * 0x40) >> 2);
				*p += val * 4 * (rslr[i] - minrslr);
			}
			phy->pctl.dqsecfg.bits.qslen = 0;
#endif
		}
		return 0;
	} else {
		return 1;
	}
}

