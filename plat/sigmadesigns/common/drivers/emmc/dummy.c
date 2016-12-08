#include <bl_common.h>
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <arch_helpers.h>
#include <firmware_image_package.h>
#include <platform_def.h>
#include <sd_private.h>


/*
 * Dummy eMMC Memory Layout
 *
 * ++++++++++++
 * +          +
 * + FIP      +
 * ++++++++++++  +0x200000 (EMMC_DRAM_FIP_BASE)
 * + RPMB     +
 * ++++++++++++  +0x1C0000 (EMMC_DRAM_RPMB_BASE)
 * + Bootpart +
 * ++++++++++++  EMMC_DRAM_BASE
 *
 * The preloaded fip.bin shall be put in dram at address EMMC_DRAM_FIP_BASE
 *
 */

#define EMMC_DRAM_BASE		0x08000000ul
#define EMMC_DRAM_FIP_BASE	(EMMC_DRAM_BASE + SD_FIP_BASE - STORAGE_IMG_BASE)
#define EMMC_DRAM_RPMB_BASE	(EMMC_DRAM_BASE + 0x1C0000)

int sd_emmc_init(void)
{
	volatile fip_toc_header_t *hdr;
	hdr = (fip_toc_header_t*)EMMC_DRAM_FIP_BASE;
	if (!(hdr->name == TOC_HEADER_NAME && hdr->serial_number != 0)) {
		/*
		 * wait until fip image is loaded
		 */
		NOTICE("please load fip.bin to dram at addr %p\n", (void*)hdr);
		do {
			inv_dcache_range((uintptr_t)hdr, sizeof(fip_toc_header_t));
		} while (!(hdr->name == TOC_HEADER_NAME && hdr->serial_number != 0));
	}
	INFO("found FIP at addr %p\n", (void*)hdr);
	return 0;
}

void sd_emmc_deinit(void)
{
}

size_t emmc_read_blocks(int lba, uintptr_t buf, size_t size)
{
	INFO("mem read, lba 0x%x buf 0x%lx size 0x%lx\n", lba, buf, size);
	memcpy((void*)buf, (void*)(EMMC_DRAM_BASE + (lba * EMMC_BLOCK_SIZE)), size);
	return size;
}

size_t emmc_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	return 0;
}


size_t emmc_boot_read_blocks(int lba, uintptr_t buf, size_t size)
{
	assert((lba * EMMC_BLOCK_SIZE + size) < (EMMC_DRAM_RPMB_BASE - EMMC_DRAM_BASE));
	return emmc_read_blocks(lba, buf, size);
}

size_t emmc_boot_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	return 0;
}

size_t emmc_rpmb_read_blocks(int lba, uintptr_t buf, size_t size)
{
	assert(size <= (EMMC_DRAM_FIP_BASE - EMMC_DRAM_RPMB_BASE));
	INFO("mem rpmb read, lba 0x%x buf 0x%lx size 0x%lx\n", lba, buf, size);
	memcpy((void*)buf, (void*)(EMMC_DRAM_RPMB_BASE + (lba * EMMC_BLOCK_SIZE)), size);
	return size;
}

size_t emmc_rpmb_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	return 0;
}
