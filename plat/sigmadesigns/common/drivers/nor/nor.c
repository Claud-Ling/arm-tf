#include <mmio.h>
#include <stdio.h>
#include <string.h>
#include <bl_common.h>
#include <debug.h>
#include <platform_def.h>
#include <sd_private.h>

int sd_nor_init(void)
{
	return 0;
}

void sd_nor_deinit(void)
{
}

size_t sd_nor_read(int lba, uintptr_t buf, size_t size)
{
	uintptr_t ofs;
	ofs = SD_NOR_BASE + (lba * FLASH_BLOCK_SIZE);
	memcpy((void*)buf, (void*)ofs, size);
	return size;
}
