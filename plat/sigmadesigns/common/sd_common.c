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
#include <arm_gic.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <xlat_tables.h>

#define MAP_SEC_DRAM	MAP_REGION_FLAT(SD_SEC_DRAM_BASE,		\
					SD_SEC_DRAM_SIZE,		\
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_NS_DRAM	MAP_REGION_FLAT(SD_NS_DRAM_BASE,		\
					SD_NS_DRAM_SIZE,		\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_NS_DRAM2	MAP_REGION_FLAT(SD_NS_DRAM_BASE2,		\
					SD_NS_DRAM_SIZE2,		\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_DEVICE	MAP_REGION_FLAT(SD_DEVICE_BASE,			\
					SD_DEVICE_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_SRAM	MAP_REGION_FLAT(SD_SRAM_BASE,			\
					SD_SRAM_SIZE,			\
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_ROM		MAP_REGION_FLAT(SD_ROM_BASE,			\
					SD_ROM_SIZE,			\
					MT_DEVICE | MT_RO | MT_SECURE)

/*
 * Table of regions for different BL stages to map using the MMU.
 * This doesn't include Trusted RAM as the 'mem_layout' argument passed to
 * sd_init_mmu_elx() will give the available subset of that,
 */
#if IMAGE_BL1
static const mmap_region_t sd_mmap[] = {
	MAP_SEC_DRAM,
	MAP_NS_DRAM,
	MAP_DEVICE,
	{0}
};
#endif

#if IMAGE_BL2
static const mmap_region_t sd_mmap[] = {
	MAP_SEC_DRAM,
	MAP_NS_DRAM,
	MAP_DEVICE,
	{0}
};
#endif

#if IMAGE_BL31
static const mmap_region_t sd_mmap[] = {
	MAP_SEC_DRAM,
	MAP_DEVICE,
	MAP_SRAM,
	{0}
};
#endif

#if IMAGE_BL32
static const mmap_region_t sd_mmap[] = {
	MAP_SEC_DRAM,
	MAP_DEVICE,
	MAP_SRAM,
	{0}
};
#endif

/*
 * The following platform setup functions are weakly defined. They
 * provide typical implementations that will be overridden by a SoC.
 */
#pragma weak sd_soc_get_mmap

/*******************************************************************************
 * Returns SD platform specific memory map regions.
 ******************************************************************************/
const mmap_region_t *sd_soc_get_mmap(void)
{
	return sd_mmap;
}

/*
 * Inspired by arm_setup_page_tables function.
 * Set up the page tables for the generic and platform-specific memory regions.
 * The extents of the generic memory regions are specified by the function
 * arguments and consist of:
 * - Trusted SRAM seen by the BL image;
 * - Code section;
 * - Read-only data section;
 * - Coherent memory region, if applicable.
 */
void sd_setup_page_tables(uintptr_t total_base,
			  size_t total_size,
			  uintptr_t code_start,
			  uintptr_t code_limit,
			  uintptr_t rodata_start,
			  uintptr_t rodata_limit
#if USE_COHERENT_MEM
			   ,
			  uintptr_t coh_start,
			  uintptr_t coh_limit
#endif
			   )
{
	/*
	 * Map the Trusted SRAM with appropriate memory attributes.
	 * Subsequent mappings will adjust the attributes for specific regions.
	 */
	VERBOSE("Trusted SRAM seen by this BL image: %p - %p\n",
		(void *) total_base, (void *) (total_base + total_size));
	mmap_add_region(total_base, total_base,
			total_size,
			MT_MEMORY | MT_RW | MT_SECURE);

	/* Re-map the code section */
	VERBOSE("Code region: %p - %p\n",
		(void *) code_start, (void *) code_limit);
	mmap_add_region(code_start, code_start,
			code_limit - code_start,
			MT_CODE | MT_SECURE);

	/* Re-map the read-only data section */
	VERBOSE("Read-only data region: %p - %p\n",
		(void *) rodata_start, (void *) rodata_limit);
	mmap_add_region(rodata_start, rodata_start,
			rodata_limit - rodata_start,
			MT_RO_DATA | MT_SECURE);

#if USE_COHERENT_MEM
	/* Re-map the coherent memory region */
	VERBOSE("Coherent region: %p - %p\n",
		(void *) coh_start, (void *) coh_limit);
	mmap_add_region(coh_start, coh_start,
			coh_limit - coh_start,
			MT_DEVICE | MT_RW | MT_SECURE);
#endif

	/* Now (re-)map the platform-specific memory regions */
	mmap_add(sd_soc_get_mmap());

	/* Create the page tables to reflect the above mappings */
	init_xlat_tables();
}

unsigned long plat_get_ns_image_entrypoint(void)
{
	return SD_NS_IMAGE_OFFSET;
}

unsigned int plat_get_syscnt_freq2(void)
{
#if SD_VELOCE
	/*
	 * in HZ
	 * NOTE: this is only for clock calibration in veloce case but not recommended
	 * for regular use, as it will lead to "rcu_sched self-detected stall" event
	 * when boot linux for veloce runs thousands times slower than usual.
	 */
	return 7200;
#else
	/*
	 * in HZ, fixed @24MHz
	 */
	return 24000000;
#endif
}
