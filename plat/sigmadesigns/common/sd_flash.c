/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <platform.h>
#include <platform_def.h>
#include <sd_private.h>

static struct flash_ops {
	int (*init)(void);	/*mandatory*/
	void (*deinit)(void);
	size_t (*blk_read)(int lba, uintptr_t buf, size_t size);	/*mandatory*/
	size_t (*blk_write)(int lba, const uintptr_t buf, size_t size);
} flash_ops;

int sd_flash_init(void)
{
	struct flash_ops *ops = &flash_ops;
#if (SD_STORAGE == SD_FLASH_MMC)
	ops->init	= sd_emmc_init;
	ops->deinit	= sd_emmc_deinit;
	ops->blk_read	= emmc_read_blocks;
	ops->blk_write	= emmc_write_blocks;
#elif (SD_STORAGE == SD_FLASH_NAND)
	ops->init	= sd_nand_init;
	ops->deinit	= sd_nand_deinit;
	ops->blk_read	= sd_nand_read;
	ops->blk_write	= NULL;
#elif (SD_STORAGE == SD_FLASH_NOR)
	ops->init	= sd_nor_init;
	ops->deinit	= sd_nor_deinit;
	ops->blk_read	= sd_nor_read;
	ops->blk_write	= NULL;
#endif
	assert(ops->init != NULL);
	return ops->init();
}

void sd_flash_deinit(void)
{
	struct flash_ops *ops = &flash_ops;
	if (ops->deinit)
		ops->deinit();
}

size_t sd_flash_read_blocks(int lba, uintptr_t buf, size_t size)
{
	struct flash_ops *ops = &flash_ops;
	assert(ops->blk_read != NULL);
	return ops->blk_read(lba, buf, size);
}

size_t sd_flash_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	struct flash_ops *ops = &flash_ops;
	if (ops->blk_write) {
		return ops->blk_write(lba, buf, size);
	} else {
		NOTICE("write_blocks not supported!\n");
		return -1;
	}
}
