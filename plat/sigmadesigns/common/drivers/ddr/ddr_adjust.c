
#include <stdio.h>
#include <debug.h>
#include <assert.h>
#include <platform_def.h>
#include <sd_private.h>
#include "umac.h"

#define put32(u) tf_printf("%x", u)

static unsigned char calc_delay_value(signed char t4, char off, char op)
{
	unsigned char val;
	if (op)
		val = t4 - (off * t4 / 16);
	else
		val = t4 + (off * t4 / 16);

	return val;
}

static int ddr_do_adjust(int uid)
{
	if (umac_is_activated(uid)) {
		unsigned int nl = UMAC_NR_OF_LANES(uid);
		const U32 quirks = umac_get_quirks(uid);
		struct umac_phy *phy = umac_get_phy(uid);
		signed char t4r, t4w;
		unsigned int signs, ofs, val, i;
		assert(phy != NULL);

		/*offsets and signs*/
		if (quirks & UMAC_QUIRK_LEGACY_TR_OFS) {
			struct umac_abt *abt = umac_get_abt(uid);
			assert(abt != NULL);
			ofs = abt->pub_tr_rw_ofs;
			signs = abt->pub_tr_rw_ofs_sign;
		} else {
			ofs = phy->pub.pub_tr_rw_ofs;
			signs = phy->pub.pub_tr_rw_ofs_sign;
		}

		/*T/4 codes*/
		t4r = phy->pub.pub_tr_rd_lcdl & 0xff;
		t4w = phy->pub.pub_tr_wr_lcdl & 0xff;
		putchar('\n');
		for (i = 0; i < nl; i++) {
			volatile PHY_DXnLCDLR1Reg *lcdlr1 = &phy->pub.phy_dx0lcdlr1 + ((i*0x40)>>2);
			val = lcdlr1->val & (~0xffff);
			/*RDQSD*/
			val |= calc_delay_value(t4r,
				((ofs>>(i*4))&0xf), ((signs>>i)&0x1)) << 8;
			/*WDQD*/
			val |= calc_delay_value(t4w,
				((ofs>>(16+i*4))&0xf), ((signs>>(4+i))&0x1));
			putchar('\n');
			putchar('V');
			put32(val);
			lcdlr1->val = val;
		}
		putchar('\n');
		return SYS_NOERROR;
	} else {
		return SYS_FAIL;
	}
}

void ddr_adjust(void)
{
	int id;
	for (id = 0; id < CONFIG_SIGMA_NR_UMACS; id++) {
		ddr_do_adjust(id);
	}
}

