/******************************************
 * Copyright 2016
 * Sigma Designs, Inc. All Rights Reserved
 * Proprietary and Confidential
 ******************************************/

/**
 * @file	sd_flash.h
 * @brief	it declares flash interfaces for Sigma Designs DTV SoCs that could be used by
 *		ATF I/O drivers.
 *		Supporting flash types: emmc/nand/nor.
 *		Flash driver is chosen according to SD_STORAGE macro.
 *
 * @author:  Tony He <tony_he@sigmadesigns.com>
 * @date:    2016/11/21
 * 
 */

#ifndef __SD_FLASH_H__
#define __SD_FLASH_H__

#include <sys/types.h>

#ifndef SD_STORAGE
  #error "No storage type! Please set it via SD_STORAGE"
#endif
#ifndef SD_BOOTDEV
  #error "No boot device! Please set it via SD_BOOTDEV"
#endif

#ifndef SD_FLASH_MMC
# define SD_FLASH_MMC	1
#endif
#ifndef SD_FLASH_NAND
# define SD_FLASH_NAND	2
#endif
#ifndef SD_FLASH_NOR
# define SD_FLASH_NOR	3
#endif

#define FLASH_OK		0
#define FLASH_ERR_IO		(-1)
#define FLASH_ERR_NOT_SUP	(-2)

#ifndef __ASSEMBLY__

#if (SD_STORAGE == SD_FLASH_MMC)
#define EMMC_BLOCK_SIZE		512
#define EMMC_BLOCK_MASK		(EMMC_BLOCK_SIZE - 1)

#define FLASH_BLOCK_SIZE 	EMMC_BLOCK_SIZE

int sd_emmc_init(void);
void sd_emmc_deinit(void);
/*
 * read data from emmc user partition
 * In:
 *	lba	- least block aligned address
 *	buf	- pointer of buffer for out data
 *	size	- bytes of read data, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t emmc_read_blocks(int lba, uintptr_t buf, size_t size);
size_t emmc_write_blocks(int lba, const uintptr_t buf, size_t size);

/*
 * read data from present selected emmc boot partition
 * In:
 *	lba	- least block aligned address
 *	buf	- pointer of buffer for out data
 *	size	- bytes of read data, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t emmc_boot_read_blocks(int lba, uintptr_t buf, size_t size);
size_t emmc_boot_write_blocks(int lba, const uintptr_t buf, size_t size);

/*
 * read data from emmc rpmb partition
 * In:
 *	lba	- least block aligned address
 *	buf	- pointer of buffer for out data
 *	size	- bytes of read data, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t emmc_rpmb_read_blocks(int lba, uintptr_t buf, size_t size);
size_t emmc_rpmb_write_blocks(int lba, const uintptr_t buf, size_t size);

#elif (SD_STORAGE == SD_FLASH_NAND) /*(SD_STORAGE == SD_FLASH_MMC)*/

# define FLASH_BLOCK_SIZE	2048	/* force 2kB (logical block for now)
					 */

int sd_nand_init(void);
void sd_nand_deinit(void);

/*
 * read data from nand flash
 * In:
 *	lba	- the least block address
 *	buf	- pointer of buffer to load read data (at least word aligned)
 *	size	- bytes of data to read, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t sd_nand_read(int lba, uintptr_t buf, size_t size);

#elif (SD_STORAGE == SD_FLASH_NOR)	/*(SD_STORAGE == SD_FLASH_NAND)*/

#define FLASH_BLOCK_SIZE	512

int sd_nor_init(void);
void sd_nor_deinit(void);

/*
 * read data from nor flash
 * In:
 *	lba	- the least block address
 *	buf	- pointer of buffer to load read data (at least word aligned)
 *	size	- bytes of data to read, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t sd_nor_read(int lba, uintptr_t buf, size_t size);

#endif	/*(SD_STORAGE == SD_FLASH_NOR)*/

#define FLASH_BLOCK_MASK	(FLASH_BLOCK_SIZE - 1)

int sd_flash_init(void);

void sd_flash_deinit(void);

/*
 * read data from flash
 * In:
 *	lba	- the least block address
 *	buf	- pointer of buffer to load read data, block_size aligned
 *	size	- bytes of data to read, block size aligned
 * Out:
 *	return bytes of read data on success, otherwise error code (<0)
 */
size_t sd_flash_read_blocks(int lba, uintptr_t buf, size_t size);

/*
 * write data to flash
 * In:
 *	lba	- the least block address
 *	buf	- pointer of buffer, block_size aligned
 *	size	- bytes of data to write, block size aligned
 * Out:
 *	return bytes of write data on success, otherwise error code (<0)
 */
size_t sd_flash_write_blocks(int lba, const uintptr_t buf, size_t size);
#endif /*__ASSEMBLY__*/

#endif /*__SD_FLASH_H__*/
