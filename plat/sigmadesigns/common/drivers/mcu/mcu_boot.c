
#include <bl_common.h>
#include <debug.h>
#include <mmio.h>
#include <assert.h>
#include <string.h>
#include <platform_def.h>
#include <sd_private.h>
#include "mcu_reg.h"

#define SD_STB_MAX_SIZE		0x10000
#define MCU_STB_MODE_A		0xAA

#define MCU_VERIFY_ENABLE	0
#define MCU_VERIFY_BUF		0x01000000ul	/*tmp buffer for image verify*/

#define MCU_BOOT_MODE	((uintptr_t)(&STB_REG(hcmd[0])))
#define MCU_SSC_CTRL	((uintptr_t)(&STB_REG(hcmd[1])))
#define MCU_BOARD_ID	((uintptr_t)(&STB_REG(hcmd[2])))
#define MCU_POWER_STATE	((uintptr_t)(&STB_REG(hcmd[3])))
#define MCU_PANEL_TYPE	((uintptr_t)(&STB_REG(hcmd[4])))

#if SD_BOOT_SCP
void release_mcu(void)
{
	INFO("Release MCU\n");
	STB_REG(mips00).val = 0;
	mmio_write_8(MCU_BOOT_MODE, MCU_STB_MODE_A);
	STB_REG(mips00).bits.mips_rstn_8051 = 1;
}

int load_mcu_config(void)
{
	int ret;
	ret = sd_load_bdconf();
	if (ret == 0) {
		uint8_t tmp;
		mmio_write_8(MCU_SSC_CTRL, 0); /*no SSC patch*/
		tmp = sd_bc_boardid();
		mmio_write_8(MCU_BOARD_ID, tmp);
		tmp = sd_bc_paneltype();
		mmio_write_8(MCU_PANEL_TYPE, tmp);
	}
	return ret;
}
#endif

void copy_mcu_data(uintptr_t buf, size_t len, size_t offset)
{
	unsigned char *start;
	int unit;

	assert(len < 0x10000 && offset <= 0x10000);
	STB_REG(mips00).val = 0;  //reset 8051
	STB_REG(sram_addr)[0] = (offset >> 8) & 0xFF;
	STB_REG(sram_addr)[1] = offset & 0xFF;
	STB_REG(mips00).bits.valid_start_addr = 1;
	STB_REG(mips00).bits.valid_start_addr = 0;
	STB_REG(mips00).bits.mips_ldsram_en = 1;

	start = (unsigned char*)buf;
	unit = len;
	while(unit--)
	{
		STB_REG(sram_data) = *(start++);
	}

	STB_REG(mips00).bits.mips_ldsram_en = 0;
}

int verify_mcu_data(uintptr_t ibuf, uintptr_t vbuf, size_t len, size_t offset)
{
	unsigned char *start;
	int unit;
	assert(len < 0x10000 && offset <= 0x10000);
	STB_REG(sram_addr)[0] = (offset >> 8) & 0xFF;
	STB_REG(sram_addr)[1] = offset & 0xFF;
	STB_REG(mips00).bits.valid_start_addr = 1;
	STB_REG(mips00).bits.valid_start_addr = 0;
	STB_REG(mips00).bits.mips_ldsram_en = 1;

	start = (unsigned char*)vbuf;
	unit = len;
	while(unit--)
	{
		*start++ = STB_REG(sram_data);
	}

	STB_REG(mips00).bits.mips_ldsram_en = 0;

	return (memcmp((void *)ibuf,(void *)vbuf,len));
}

#define MCU_HEADER_SIZE (0x4)
#define MCU_HDR_MAGIC1	'H'
#define MCU_HDR_MAGIC2	'D'
int get_mcu_total_size(unsigned char *buf, int *head_size)
{
	int nsize = 0;
	if (MCU_HDR_MAGIC1 == *buf && MCU_HDR_MAGIC2 == *(buf+1)) {
		nsize = *(buf+2) + ((*(buf+3)) << 8);	/*LE*/
		if (nsize > SD_STB_MAX_SIZE)
			nsize = SD_STB_MAX_SIZE;	/*TODO*/
		*head_size = MCU_HEADER_SIZE;	/*skip header*/
		nsize += MCU_HEADER_SIZE;
	} else {
		nsize = SD_STB_MAX_SIZE; /*default*/
		*head_size = 0;
	}
	return nsize;
}

int sd_relocate_mcu(void *img, unsigned int img_size)
{
	if (!STB_REG(mips20).bits.reg_start) {
#ifdef MCU_WDOG_ENABLE
		INFO("wait mcu wdog reset...\n");
		STB_REG(read_cycle) = 0x80;
		while((STB_REG(read_cycle) & 0x80) != 0);
		INFO("done\n");
		mdelay(10);
#endif
		/*relocate code to sram*/
		assert(img && img_size <= SD_STB_MAX_SIZE);
		copy_mcu_data((uintptr_t)img, img_size, 0);
#if MCU_VERIFY_ENABLE
		if (verify_mcu_data((uintptr_t)img, MCU_VERIFY_BUF, img_size, 0) != 0) {
			ERROR("load mcu error!\n");
		}
#endif
#if SD_BOOT_SCP
		load_mcu_config();
		release_mcu();
#endif
	} else {
		INFO("skip boot mcu\n");
	}
	return 0;
}
