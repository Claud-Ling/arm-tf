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

#ifndef __SD_DEF_H__
#define __SD_DEF_H__

/*
 * SoC constants
 */
#define CONFIG_SIGMA_SOC_UNION

/*******************************************************************************
 * This value is used by the PSCI implementation during the `SYSTEM_SUSPEND`
 * call as the `state-id` field in the 'power state' parameter.
 ******************************************************************************/
#define PSTATE_ID_SOC_POWERDN	0xD

/*
 * secondary boot register (32-bit)
 *
 * +--------------------------------+
 * |31                        4|3  0|
 * +---------------------------+----+
 * | physical entry address    | id |
 * +---------------------------+----+
 *
 * Note:
 * entry address must be 16-byte aligned at least
 */
#define AUX_BOOT_ADDR_REG		0xFB00F07C	/*this belongs to sectimer target,
							 *non-accessible to NS world
							 */
#define AUX_BOOT_ID_BITS		4
#define AUX_BOOT_ID_MASK		((1 << AUX_BOOT_ID_BITS) - 1)
#define AUX_BOOT_ADDR_MASK		(~AUX_BOOT_ID_MASK)

#define MK_AUX_BOOT_VAL(ep, id) 	(((ep) & AUX_BOOT_ADDR_MASK) | \
					 ((id) & AUX_BOOT_ID_MASK))
#define AUX_BOOT_ADDR(val)		((val) & AUX_BOOT_ADDR_MASK)
#define AUX_BOOT_ID(val)		((val) & AUX_BOOT_ID_MASK)

/*
 * Memory related constants
 */

/* Always assume DDR is 1GB size. */
#define SD_DRAM_BASE			0x00000000
#define SD_DRAM_SIZE			0x40000000

/* World Share Memory 2M@256M */
#define SD_WSM_BASE			0x10000000
#define SD_WSM_SIZE			0x00200000

/* Secure DRAM 14M@258M */
#define SD_SEC_DRAM_BASE		0x10200000
#define SD_SEC_DRAM_SIZE		0x00E00000

/* Non-secure DRAM 256M@0M, 752M@272M*/
#define SD_NS_DRAM_BASE			SD_DRAM_BASE
#define SD_NS_DRAM_SIZE			(SD_SEC_DRAM_BASE - SD_DRAM_BASE)
#define SD_NS_DRAM_BASE2		(SD_SEC_DRAM_BASE + SD_SEC_DRAM_SIZE)
#define SD_NS_DRAM_SIZE2		(SD_DRAM_SIZE - SD_NS_DRAM_SIZE - SD_SEC_DRAM_SIZE)

#define SD_DEVICE_BASE			0xF0000000
#define SD_DEVICE_SIZE			0x0FF00000

#define SD_SRAM_BASE			0xFFF00000
#define SD_SRAM_SIZE			0x00012000

#define SD_ROM_BASE			0xFFFF0000
#define SD_ROM_SIZE			0x00010000

#define STORAGE_IMG_BASE		0
#define SD_FIP_BASE			(STORAGE_IMG_BASE + (2 << 20))
#define SD_FIP_MAX_SIZE			(4 << 20)

/* flash temp buffer 2M@270M */
#define SD_FLASH_DATA_BASE		(SD_SEC_DRAM_BASE + SD_SEC_DRAM_SIZE - SD_FLASH_DATA_SIZE)
#define SD_FLASH_DATA_SIZE		0x00200000
#if (SD_FLASH_DATA_BASE < (SD_SEC_DRAM_BASE + 0x100000))
# error "Small Secure DRAM!!"
#endif

/* uboot @32M */
#define SD_NS_IMAGE_OFFSET		0x02000000

/*
 * Serial related constants
 */
#define SD_BOOT_UART_BASE		0xFB005100
#define SD_UART_CLK_HZ			192000
#define SD_UART_BAUDRATE		115200
#define SD_CRASH_UART_BASE		SD_BOOT_UART_BASE

/*
 * Timer constants
 */
#define SD_TIMER_BASE			0xF5027000
#define SD_TIMER_CLOCK_MHZ		200	/*200MHz*/

/*
 * eMMC constants
 */
#define SD_MMC_BASE			0xFB00A000
#define SD_MMC_BOOT_SIZE		0x200000

/*
 * NAND constants
 */
#define SD_TSB_BASE			0xFB002000
#define SD_TSB1_BASE			0xFC000000

/*
 * NOR constants
 */
#define SD_NOR_BASE			0xFC000000

/*
 * OTP constants
 */
#define SD_TURING_BASE			0xF1040000
#define SD_OTP_FUSE_BASE		(SD_TURING_BASE + 0x1000)
#define SD_OTP_DATA_BASE		(SD_OTP_FUSE_BASE + 0x100)

/*
 * GIC-500 & interrupt handling related constants
 */
#define BASE_GICD_BASE			0xFFC00000
#define BASE_GICR_BASE			0xFFC40000	/*core 0*/

#define ARM_IRQ_SEC_PHY_TIMER		29

#define ARM_IRQ_SEC_SGI_0		8
#define ARM_IRQ_SEC_SGI_1		9
#define ARM_IRQ_SEC_SGI_2		10
#define ARM_IRQ_SEC_SGI_3		11
#define ARM_IRQ_SEC_SGI_4		12
#define ARM_IRQ_SEC_SGI_5		13
#define ARM_IRQ_SEC_SGI_6		14
#define ARM_IRQ_SEC_SGI_7		15

#define MAX_INTR_EL3			160

/*
 * UMAC constants
 */
#define CONFIG_REG_BASE_UMAC0		0xF5021000
#define CONFIG_SIGMA_NR_UMACS		1
#define CONFIG_UMAC_DQS_NEW_METHOD	/*new DQS gating method*/
#define CONFIG_SKIP_DDR_ADJUST		/*skip ddr adjust*/

/*
 * Register table constants
 */
#define SD_RT_SIZE			8192	/*8k, must in decimal*/

/*
 * DCSN constants
 */
#define SD_PLF_DCSN_SEC_BASE		0xF5002000
#define SD_AV_DCSN_SEC_BASE		0xF0016000
#define SD_DISP_DCSN_SEC_BASE		0xFA000000

/*
 * PMAN constants
 */
#define SD_PMAN_SEC0_BASE		0xF5005000
#define PMAN_SEC_GROUP_MAX		CONFIG_SIGMA_NR_UMACS

/*
 * Standby constants
 */
#define SD_STB_REG_BASE			0xF5000000
#define SD_STB_MAX_SIZE			0x10000	/*64k*/

/*
 * Board configure constants
 */
#if (SD_STORAGE == SD_FLASH_MMC)
# define SD_BDCONF_BASE			0x90000 /*576k at emmc bootpart*/
#elif (SD_STORAGE == SD_FLASH_NAND)
# define SD_BDCONF_BASE			(STORAGE_IMG_BASE + 0x1800000) /*24M*/
#else
# error "not support bdconf in selected storage type!"
#endif
#define SD_BDCONF_SIZE			FLASH_BLOCK_SIZE /*one block is enough*/

#endif /*_SD_DEF_H___*/
