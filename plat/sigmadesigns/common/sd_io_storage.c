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

#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <firmware_image_package.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_fip.h>
#include <io_memmap.h>
#include <io_storage.h>
#include <mmio.h>
#include <platform_def.h>
#include <semihosting.h>	/* For FOPEN_MODE_... */
#include <string.h>
#include <tbbr_img_def.h>	/* For XX_IMAGE_ID */
#include <sd_private.h>

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const io_dev_connector_t *flash_dev_con;
static uintptr_t flash_dev_handle;
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;

static int check_flash(const uintptr_t spec);
static int check_fip(const uintptr_t spec);

static const io_block_spec_t flash_fip_spec = {
	.offset		= SD_FIP_BASE,
	.length		= SD_FIP_MAX_SIZE,
};

static const io_block_dev_spec_t flash_dev_spec = {
	/* It's used as temp buffer in block driver. */
	.buffer		= {
		.offset	= SD_FLASH_DATA_BASE,
		.length	= SD_FLASH_DATA_SIZE,
	},
	.ops		= {
		.read	= sd_flash_read_blocks,
		.write	= sd_flash_write_blocks,
	},
	.block_size	= FLASH_BLOCK_SIZE,
};

static uintptr_t flash_boot_dev_handle;

static const io_block_spec_t flash_boot_spec = {
	.offset		= 0,
	.length		= FLASH_BOOT_SIZE,
};

static const io_block_dev_spec_t flash_boot_dev_spec = {
	/* It's used as temp buffer in block driver. */
	.buffer		= {
		.offset	= SD_FLASH_DATA_BASE,
		.length	= SD_FLASH_DATA_SIZE,
	},
	.ops		= {
		.read	= sd_flash_boot_read_blocks,
		.write	= sd_flash_boot_write_blocks,
	},
	.block_size	= FLASH_BLOCK_SIZE,
};

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static const io_uuid_spec_t scp_bl2_uuid_spec = {
	.uuid = UUID_SCP_FIRMWARE_SCP_BL2,
};

static const struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		&flash_dev_handle,
		(uintptr_t)&flash_fip_spec,
		check_flash
	},
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		check_fip
	},
	[SCP_BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_bl2_uuid_spec,
		check_fip
	},
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		check_fip
	},
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		check_fip
	},
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		check_fip
	},
};

static int check_flash(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(flash_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(flash_dev_handle, spec, &local_handle);
		if (result == 0) {
			io_close(local_handle);
		}
	}
	return result;
}

static int check_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

void sd_io_setup(void)
{
	int result;

	result = register_io_dev_block(&flash_dev_con);
	assert(result == 0);

	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);

	result = io_dev_open(flash_dev_con, (uintptr_t)&flash_dev_spec,
			     &flash_dev_handle);
	assert(result == 0);

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);

	result = io_dev_open(flash_dev_con, (uintptr_t)&flash_boot_dev_spec,
			     &flash_boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy
 */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	assert(policy->check != NULL);
	result = policy->check(policy->image_spec);
	assert(result == 0);

	*image_spec = policy->image_spec;
	*dev_handle = *(policy->dev_handle);

	return result;
}

/*******************************************************************************
 * Generic function to load a raw image at a specific address.
 *
 * Returns 0 on success, a negative error code otherwise.
 ******************************************************************************/
static int load_raw_image(uintptr_t dev_handle,
			uintptr_t image_spec,
			uintptr_t image_base,
			size_t image_size)
{
	uintptr_t image_handle;
	size_t bytes_read;
	int io_result;

	/* Attempt to access the image */
	io_result = io_open(dev_handle, image_spec, &image_handle);
	if (io_result != 0) {
		WARN("Failed to access raw image (%i)\n",
			io_result);
		return io_result;
	}

	INFO("Loading raw image from 0x%lx to address %p\n",
		((io_block_spec_t*)image_spec)->offset, (void *) image_base);

	/* We have enough space so load the image now */
	/* TODO: Consider whether to try to recover/retry a partially successful read */
	io_result = io_read(image_handle, image_base, image_size, &bytes_read);
	if ((io_result != 0) || (bytes_read < image_size)) {
		WARN("Failed to load raw image (%i)\n", io_result);
		goto exit;
	}

	/*
	 * File has been successfully loaded.
	 * Flush the image to main memory so that it can be executed later by
	 * any CPU, regardless of cache and MMU state.
	 * When TBB is enabled the image is flushed later, after image
	 * authentication.
	 */
	flush_dcache_range(image_base, image_size);

	INFO("Raw image loaded at address %p, size = 0x%zx\n",
		(void *) image_base, image_size);

exit:
	io_close(image_handle);
	/* Ignore improbable/unrecoverable error in 'close' */

	/* TODO: Consider maintaining open device connection from this bootloader stage */
	io_dev_close(dev_handle);
	/* Ignore improbable/unrecoverable error in 'dev_close' */

	return io_result;
}

/*
 * load raw image from boot device
 */
int sd_boot_load_raw_image(uintptr_t image_spec, uintptr_t image_base, size_t image_size)
{
	return load_raw_image(flash_boot_dev_handle, image_spec, image_base, image_size);
}
