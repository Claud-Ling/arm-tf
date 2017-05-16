/*
 * Copyright (c) 2014-2015, ARM Limited and Contributors. All rights reserved.
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

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include <common_def.h>
#include <tbbr_img_def.h>
#include <sd_def.h>

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/

/* Size of cacheable stacks */
#if IMAGE_BL1
#define PLATFORM_STACK_SIZE		0x2000
#elif IMAGE_BL2
#define PLATFORM_STACK_SIZE		0x2000
#elif IMAGE_BL31
#define PLATFORM_STACK_SIZE		0x2000
#elif IMAGE_BL32
#define PLATFORM_STACK_SIZE		0x440
#endif

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_CLUSTER_COUNT		1
#define PLATFORM_MAX_CPUS_PER_CLUSTER	4
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * PLATFORM_MAX_CPUS_PER_CLUSTER)

#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CORE_COUNT + PLATFORM_CLUSTER_COUNT + 1)
#define PLAT_MAX_RET_STATE		1
#define PLAT_MAX_OFF_STATE		2

#define MAX_IO_DEVICES			3
#define MAX_IO_HANDLES			4
/* eMMC RPMB and eMMC User Data */
#define MAX_IO_BLOCK_DEVICES		2

/*
 * GIC related constants (GIC-500)
 */
#define PLAT_ARM_GICD_BASE		BASE_GICD_BASE
#define PLAT_ARM_GICR_BASE		BASE_GICR_BASE	/*for core0*/

/*
 * Define a list of Group 1 Secure and Group 0 interrupts as per GICv3
 * terminology. On a GICv2 system or mode, the lists will be merged and treated
 * as Group 0 interrupts.
 */
#define PLAT_ARM_G1S_IRQS	ARM_IRQ_SEC_PHY_TIMER,	\
				ARM_IRQ_SEC_SGI_1,	\
				ARM_IRQ_SEC_SGI_2,	\
				ARM_IRQ_SEC_SGI_3,	\
				ARM_IRQ_SEC_SGI_4,	\
				ARM_IRQ_SEC_SGI_5,	\
				ARM_IRQ_SEC_SGI_7

#define PLAT_ARM_G0_IRQS	ARM_IRQ_SEC_SGI_0,	\
				ARM_IRQ_SEC_SGI_6

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
/*
 * BL1 specific defines.
 *
 * Both loader and BL1_RO region stay in SRAM since they are used to simulate
 * ROM.
 *
 * ++++++++++  0xFFF0_0000
 * + BL1_RO +
 * ++++++++++  0xFFF0_C000
 * + BL1_RW +
 * ++++++++++  0xFFF1_2000
 */
#if (SD_BOOTDEV == SD_FLASH_NOR)
#define BL1_RO_BASE			(SD_ROM_BASE)
#define BL1_RO_LIMIT			(SD_ROM_BASE + 0xc000)
#define BL1_RW_BASE			(SD_SRAM_BASE + 0xc000)
#define BL1_RW_LIMIT			(SD_SRAM_BASE + SD_SRAM_SIZE)
#define BL1_RW_SIZE			(BL1_RW_LIMIT - BL1_RW_BASE)
#else
#define BL1_RO_BASE			(SD_SRAM_BASE)
#define BL1_RO_LIMIT			(SD_SRAM_BASE + 0xc000)
#define BL1_RW_BASE			(BL1_RO_LIMIT)
#define BL1_RW_LIMIT			(SD_SRAM_BASE + SD_SRAM_SIZE)
#define BL1_RW_SIZE			(BL1_RW_LIMIT - BL1_RW_BASE)
#endif

/*
 * BL2 specific defines.
 */
/*
 * SEC_DRAM Layout
 *
 * ++++++++++++  SD_SEC_DRAM_LIMIT
 * + SEC DRAM +
 * +          +
 * ++++++++++++  +0x160000
 * + BL32(TOS)+
 * ++++++++++++  +0x060000
 * + BL31     +
 * ++++++++++++  +0x020000
 * + BL2      +
 * ++++++++++++  SD_SEC_DRAM_BASE
 *
 */

#define BL2_BASE			SD_SEC_DRAM_BASE
#define BL2_LIMIT			(SD_SEC_DRAM_BASE + 0x60000)
#define BL2_SIZE			0x20000

/*******************************************************************************
 * BL31 specific defines.
 ******************************************************************************/
#define BL31_BASE			(SD_SEC_DRAM_BASE + BL2_SIZE)
#define BL31_LIMIT			BL2_LIMIT

/*******************************************************************************
 * BL32 specific defines.
 ******************************************************************************/
#define BL32_SRAM_BASE			SD_SRAM_BASE
#define BL32_SRAM_LIMIT			BL32_SRAM_BASE	/* N/A */

#define BL32_DRAM_BASE			BL31_LIMIT
#define BL32_DRAM_LIMIT			(BL32_DRAM_BASE + 0x100000)

#define BL32_BASE			BL32_DRAM_BASE
#define BL32_LIMIT			BL32_DRAM_LIMIT

/*
 * SCP_BL2 specific defines.
 * In union, SCP_BL2 means MCU firmware. It's loaded into the temporary buffer
 * same as BL31. Then BL2 will parse the sections and loaded them into
 * predefined separated buffers.
 */
#if SD_LOAD_SCP_IMAGES
#define SCP_BL2_BASE			BL31_BASE
#define SCP_BL2_LIMIT			(SCP_BL2_BASE + SCP_BL2_SIZE)
#define SCP_BL2_SIZE			0x10000
#endif

/*******************************************************************************
 * TSP  specific defines.
 ******************************************************************************/
#define TSP_SEC_MEM_BASE		BL32_BASE
#define TSP_SEC_MEM_SIZE		(BL32_LIMIT - BL32_BASE)

/* ID of the secure physical generic timer interrupt used by the TSP */
#define TSP_IRQ_SEC_PHY_TIMER		ARM_IRQ_SEC_PHY_TIMER

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define ADDR_SPACE_SIZE		(1ull << 32)
#if IMAGE_BL1
# define MAX_XLAT_TABLES	3
# define MAX_MMAP_REGIONS	8
#else
# define MAX_XLAT_TABLES	4
# define MAX_MMAP_REGIONS	16
#endif

/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE	(1 << CACHE_WRITEBACK_SHIFT)

#endif /* __PLATFORM_DEF_H__ */
