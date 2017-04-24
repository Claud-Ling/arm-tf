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

#include <bl_common.h>
#include <console.h>
#include <platform_def.h>
#include <platform_tsp.h>
#include <sd_private.h>
#include <xlat_tables.h>

#define BL32_END (unsigned long)(&__BL32_END__)

/* Weak definitions may be overridden in specific platform */
#pragma weak tsp_early_platform_setup
#pragma weak tsp_platform_setup
#pragma weak tsp_plat_arch_setup

/*******************************************************************************
 * Initialize the UART
 ******************************************************************************/
void tsp_early_platform_setup(void)
{
	/* Initialize the console to provide early debug support */
	console_init(SD_BOOT_UART_BASE,
		     SD_UART_CLK_HZ,
		     SD_UART_BAUDRATE);
}

/*******************************************************************************
 * Perform platform specific setup placeholder
 ******************************************************************************/
void tsp_platform_setup(void)
{
	plat_sd_gic_driver_init();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the MMU
 ******************************************************************************/
void tsp_plat_arch_setup(void)
{
	sd_setup_page_tables(BL32_BASE,
			     BL32_END - BL32_BASE,
			     BL_CODE_BASE,
			     BL_CODE_LIMIT,
			     BL_RO_DATA_BASE,
			     BL_RO_DATA_LIMIT
#if USE_COHERENT_MEM
			     , BL_COHERENT_RAM_BASE,
			     BL_COHERENT_RAM_LIMIT
#endif
			     );
	enable_mmu_el1(0);
}
