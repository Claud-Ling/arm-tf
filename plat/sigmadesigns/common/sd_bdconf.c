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

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <io_storage.h>
#include <platform.h>
#include <platform_def.h>
#include <sd_private.h>

/*
 * Board Configureation body
 * FW just interests in the top few bytes
 */
#define BC_VERSION	0
#define BC_BOARD_ID	1
#define BC_USB_PORT	2
#define BC_PANEL_TYPE	3

static uint8_t bdconf_data[SD_BDCONF_SIZE];

static const io_block_spec_t bdconf_spec = {
	.offset		= SD_BDCONF_BASE,
	.length		= SD_ALIGNTONEXT(SD_BDCONF_SIZE, FLASH_BLOCK_SIZE),
};

int sd_load_bdconf(void)
{
	int e = 0;
	INFO("BL2: Loading bdconf\n");
	e = sd_boot_load_raw_image((uintptr_t)&bdconf_spec,
				(uintptr_t)bdconf_data,
				SD_BDCONF_SIZE);
	if (e != 0) {
		ERROR("failed to load bdconf!\n");
	}

	return e;
}

int sd_bc_boardid(void)
{
	return bdconf_data[BC_BOARD_ID];
}

int sd_bc_paneltype(void)
{
	return bdconf_data[BC_PANEL_TYPE];
}
