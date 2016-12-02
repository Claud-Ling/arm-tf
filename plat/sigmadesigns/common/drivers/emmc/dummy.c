#include <bl_common.h>
#include <debug.h>
#include <string.h>
#include <platform_def.h>

#include <sd_private.h>

#define EMMC_DRAM_FIP_BASE	0x08000000ul

int sd_emmc_init(void)
{
	return 0;
}

void sd_emmc_deinit(void)
{
}

size_t emmc_read_blocks(int lba, uintptr_t buf, size_t size)
{
	NOTICE("mem read, lba 0x%x buf 0x%lx size 0x%lx\n", lba, buf, size);
	memcpy((void*)buf, (void*)(EMMC_DRAM_FIP_BASE + (lba * EMMC_BLOCK_SIZE)), size);
	return size;
}

size_t emmc_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	return 0;
}


size_t emmc_boot_read_blocks(int lba, uintptr_t buf, size_t size)
{
	return emmc_read_blocks(lba, buf, size);
}

size_t emmc_boot_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	return 0;
}

size_t emmc_rpmb_read_blocks(int lba, uintptr_t buf, size_t size)
{
	return emmc_read_blocks(lba, buf, size);
}

size_t emmc_rpmb_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	return 0;
}
