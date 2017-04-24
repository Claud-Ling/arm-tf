/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
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
#include <console.h>
#include <platform_def.h>
#include <sd_private.h>
#include <xlat_tables.h>
#include "../../bl1/bl1_private.h"

/* Data structure which holds the extents of the trusted SRAM for BL1*/
static meminfo_t bl1_tzram_layout;
extern void bl1_entrypoint(void);

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	assert(BL2_SIZE <= (BL2_LIMIT - BL2_BASE));
	/*
	 * override bl1_tzram_layout with sec_dram
	 * bl2 resides in sec_dram and all sec_dram are free to bl2 at this point
	 */
	bl1_tzram_layout.total_base = BL2_BASE;
	bl1_tzram_layout.total_size = BL2_LIMIT - BL2_BASE;

	/* Calculate how much RAM BL1 is using and how much remains free */
	bl1_tzram_layout.free_base = bl1_tzram_layout.total_base;
	bl1_tzram_layout.free_size = bl1_tzram_layout.total_size;
	return &bl1_tzram_layout;
}

/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void bl1_early_platform_setup(void)
{
	const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;

	/* Initialize the console to provide early debug support */
	console_init(SD_BOOT_UART_BASE,
		     SD_UART_CLK_HZ,
		     SD_UART_BAUDRATE);

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = BL1_RW_BASE;
	bl1_tzram_layout.total_size = BL1_RW_SIZE;

	/* Calculate how much RAM BL1 is using and how much remains free */
	bl1_tzram_layout.free_base = BL1_RW_BASE;
	bl1_tzram_layout.free_size = BL1_RW_SIZE;
	reserve_mem(&bl1_tzram_layout.free_base, &bl1_tzram_layout.free_size,
		    BL1_RAM_BASE, bl1_size);

#if !COLD_BOOT_SINGLE_CPU && (SD_BOOTDEV != SD_FLASH_NOR)
	/* Leading secondary cores into bl1 unless boot from NOR */
	sd_wakeup_secondary((uintptr_t)bl1_entrypoint, 0);
#endif
}

/******************************************************************************
 * Perform the very early platform specific architecture setup.  This only
 * does basic initialization. Later architectural setup (bl1_arch_setup())
 * does not do anything platform specific.
 *****************************************************************************/
void bl1_plat_arch_setup(void)
{
/* Not enable mmu for SPI boot case, for nor flash doesn't support burst access */
#if (SD_BOOTDEV != SD_FLASH_NOR)
	sd_setup_page_tables(bl1_tzram_layout.total_base,
			     bl1_tzram_layout.total_size,
			     BL_CODE_BASE,
			     BL1_CODE_LIMIT,
			     BL1_RO_DATA_BASE,
			     BL1_RO_DATA_LIMIT
#if USE_COHERENT_MEM
			     , BL_COHERENT_RAM_BASE,
			     BL_COHERENT_RAM_LIMIT
#endif
			     );
#ifdef AARCH32
	enable_mmu_secure(0);
#else
	enable_mmu_el3(0);
#endif /* AARCH32 */

#endif /* SD_BOOTDEV != SD_FLASH_NOR */
}

/*
 * Function which will perform any remaining platform-specific setup that can
 * occur after the MMU and data cache have been enabled.
 */
void bl1_platform_setup(void)
{
	plat_sd_security_setup();	/*shall have it in bl2 in case bl1 is treated as ROM*/
	sd_timer_init();
	dsb();
	sd_ddr_init();
	sd_flash_init();
	sd_io_setup();
}

/*******************************************************************************
 * Function that takes a memory layout into which BL2 has been loaded and
 * populates a new memory layout for BL2 that ensures that BL1's data sections
 * resident in secure RAM are not visible to BL2.
 ******************************************************************************/
void bl1_init_bl2_mem_layout(const meminfo_t *bl1_mem_layout,
			     meminfo_t *bl2_mem_layout)
{
	assert(bl1_mem_layout != NULL);
	assert(bl2_mem_layout != NULL);
	/* Check that BL1's memory is lying outside of the free memory */
	assert((BL1_RAM_LIMIT <= bl1_mem_layout->free_base) ||
	       (BL1_RAM_BASE >= bl1_mem_layout->free_base +
				bl1_mem_layout->free_size));

	/*
	 * For union, bl2 resides in DRAM which is irrelavant to bl1
	 */
	*bl2_mem_layout = *bl1_mem_layout;

	flush_dcache_range((unsigned long)bl2_mem_layout, sizeof(meminfo_t));
}

/*******************************************************************************
 * Before calling this function BL2 is loaded in memory and its entrypoint
 * is set by load_image. This is a placeholder for the platform to change
 * the entrypoint of BL2 and set SPSR and security state.
 * On ARM standard platforms we only set the security state of the entrypoint
 ******************************************************************************/
void bl1_plat_set_bl2_ep_info(image_info_t *bl2_image,
				entry_point_info_t *bl2_ep)
{
	SET_SECURITY_STATE(bl2_ep->h.attr, SECURE);
	bl2_ep->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
}
