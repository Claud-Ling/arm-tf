#include <mmio.h>
#include <stdint.h>

#ifndef __PINSHARE_H__
#define __PINSHARE_H__

/*
 * emmc pin share
 */
#define PLAT_PIN_SHARE_FAD0	0xf500ee27
# define FAD0_MASK		0x70
# define FAD0_NAND_DAT0		0x00
# define FAD0_SDIO_DAT0		0x10

#define PLAT_PIN_SHARE_FAD1	0xf500ee29
# define FAD1_MASK		0x07
# define FAD1_NAND_DAT1		0x00
# define FAD1_SDIO_DAT1		0x01

#define PLAT_PIN_SHARE_FAD2	0xf500ee29
# define FAD2_MASK		0x70
# define FAD2_NAND_DAT2		0x00
# define FAD2_SDIO_DAT2		0x10

#define PLAT_PIN_SHARE_FAD3	0xf500ee2a
# define FAD3_MASK		0x07
# define FAD3_NAND_DAT3		0x00
# define FAD3_SDIO_DAT3		0x01

#define PLAT_PIN_SHARE_FAD4	0xf500ee2a
# define FAD4_MASK		0x70
# define FAD4_NAND_DAT4		0x00
# define FAD4_SDIO_DAT4		0x10

#define PLAT_PIN_SHARE_FAD5	0xf500ee2b
# define FAD5_MASK		0x07
# define FAD5_NAND_DAT5		0x00
# define FAD5_SDIO_DAT5		0x01

#define PLAT_PIN_SHARE_FAD6	0xf500ee2b
# define FAD6_MASK		0x70
# define FAD6_NAND_DAT6		0x00
# define FAD6_SDIO_DAT6		0x10

#define PLAT_PIN_SHARE_FAD7	0xf500ee2c
# define FAD7_MASK		0x07
# define FAD7_NAND_DAT7		0x00
# define FAD7_SDIO_DAT7		0x01

#define PLAT_PIN_SHARE_CLE	0xf500ee2c
# define CLE_MASK		0x70
# define CLE_NAND_CLE		0x00
# define CLE_SDIO_SDCLK		0x10

#define PLAT_PIN_SHARE_ALE	0xf500ee2d
# define ALE_MASK		0x07
# define ALE_NAND_ALE		0x00
# define ALE_SDIO_CMD		0x01

#define PLAT_PIN_SHARE_FOE	0xf500ee2d
# define FOE_MASK		0x70
# define FOE_NAND_FOE		0x00

#ifndef __ASSEMBLY__
static inline void config_pin_share(uint32_t reg, uint8_t mask, uint8_t function)
{
	uint8_t _val;

	_val = mmio_read_8(reg);
	_val &= ~mask;
	_val |= function;
	mmio_write_8(reg, _val);
}

#endif	/*__ASSEMBLY__*/
#endif /*__PINSHARE_H__*/
