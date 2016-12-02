#include <mmio.h>
#include <stdio.h>
#include <string.h>
#include <bl_common.h>
#include <debug.h>
#include <platform_def.h>
#include <sd_private.h>
#include <sd_otp.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define SDHC_REG(offset) (SD_MMC_BASE+(offset))

/*EMMC PARTITION ACESS*/
#define EMMC_PART_USER	0
#define EMMC_PART_BOOT1	1
#define EMMC_PART_BOOT2	2
#define EMMC_PART_RPMB	3
#define EMMC_PART_GP1	4
#define EMMC_PART_GP2	5
#define EMMC_PART_GP3	6
#define EMMC_PART_GP4	7

#define EMMC_PART_BOOT	8	/*current selected boot part*/

#define SD_VERSION_SD	0x20000
#define SD_VERSION_2	(SD_VERSION_SD | 0x20)
#define SD_VERSION_1_0	(SD_VERSION_SD | 0x10)
#define SD_VERSION_1_10	(SD_VERSION_SD | 0x1a)
#define MMC_VERSION_MMC		0x10000
#define MMC_VERSION_UNKNOWN	(MMC_VERSION_MMC)
#define MMC_VERSION_1_2		(MMC_VERSION_MMC | 0x12)
#define MMC_VERSION_1_4		(MMC_VERSION_MMC | 0x14)
#define MMC_VERSION_2_2		(MMC_VERSION_MMC | 0x22)
#define MMC_VERSION_3		(MMC_VERSION_MMC | 0x30)
#define MMC_VERSION_4		(MMC_VERSION_MMC | 0x40)

#define MMC_MODE_HS		0x001
#define MMC_MODE_HS_52MHz	0x010
#define MMC_MODE_4BIT		0x100
#define MMC_MODE_8BIT		0x200
#define MMC_MODE_SPI		0x400
#define MMC_MODE_HC		0x800

#define SD_DATA_4BIT		0x00040000

#define IS_SD(x) (x->version & SD_VERSION_SD)

#define MMC_DATA_READ		1
#define MMC_DATA_WRITE		2

#define NO_CARD_ERR		-16 /* No SD/MMC card inserted */
#define UNUSABLE_ERR		-17 /* Unusable Card */
#define COMM_ERR		-18 /* Communications Error */
#define TIMEOUT			-19

#define MMC_CMD_GO_IDLE_STATE		0
#define MMC_CMD_SEND_OP_COND		1
#define MMC_CMD_ALL_SEND_CID		2
#define MMC_CMD_SET_RELATIVE_ADDR	3
#define MMC_CMD_SET_DSR			4
#define MMC_CMD_SWITCH			6
#define MMC_CMD_SELECT_CARD		7
#define MMC_CMD_SEND_EXT_CSD		8
#define MMC_CMD_SEND_CSD		9
#define MMC_CMD_SEND_CID		10
#define MMC_CMD_STOP_TRANSMISSION	12
#define MMC_CMD_SEND_STATUS		13
#define MMC_CMD_SET_BLOCKLEN		16
#define MMC_CMD_READ_SINGLE_BLOCK	17
#define MMC_CMD_READ_MULTIPLE_BLOCK	18
#define MMC_CMD_WRITE_SINGLE_BLOCK	24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_ERASE_GROUP_START	35
#define MMC_CMD_ERASE_GROUP_END		36
#define MMC_CMD_ERASE			38
#define MMC_CMD_APP_CMD			55
#define MMC_CMD_SPI_READ_OCR		58
#define MMC_CMD_SPI_CRC_ON_OFF		59

#define SD_CMD_SEND_RELATIVE_ADDR	3
#define SD_CMD_SWITCH_FUNC		6
#define SD_CMD_SEND_IF_COND		8

#define SD_CMD_APP_SET_BUS_WIDTH	6
#define SD_CMD_ERASE_WR_BLK_START	32
#define SD_CMD_ERASE_WR_BLK_END		33
#define SD_CMD_APP_SEND_OP_COND		41
#define SD_CMD_APP_SEND_SCR		51

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY	0x00020000
#define SD_HIGHSPEED_SUPPORTED	0x00020000

#define MMC_HS_TIMING		0x00000100
#define MMC_HS_52MHZ		0x2

#define OCR_BUSY		0x80000000
#define OCR_HCS			0x40000000
#define OCR_VOLTAGE_MASK	0x007FFF80
#define OCR_ACCESS_MODE		0x60000000

#define SECURE_ERASE		0x80000000

#define MMC_STATUS_MASK		(~0x0206BF7F)
#define MMC_STATUS_RDY_FOR_DATA (1 << 8)
#define MMC_STATUS_CURR_STATE	(0xf << 9)
#define MMC_STATUS_ERROR	(1 << 19)

#define MMC_STATE_PRG		(7 << 9)

#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

#define MMC_SWITCH_MODE_CMD_SET		0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS	0x01 /* Set bits in EXT_CSD byte
						addressed by index which are
						1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS	0x02 /* Clear bits in EXT_CSD byte
						addressed by index, which are
						1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE	0x03 /* Set target byte to value */

#define SD_SWITCH_CHECK		0
#define SD_SWITCH_SWITCH	1

/*
 * EXT_CSD fields
 */
#define EXT_CSD_PARTITIONING_SUPPORT	160	/* RO */
#define EXT_CSD_ERASE_GROUP_DEF		175	/* R/W */
#define EXT_CSD_PART_CONF		179	/* R/W */
# define EXT_CSD_BOOT_ACK_SHIFT			6
# define EXT_CSD_BOOT_ACK_MASK			(1 << EXT_CSD_BOOT_ACK_SHIFT)
# define EXT_CSD_BOOT_PARTITION_ENABLE_SHIFT	3
# define EXT_CSD_BOOT_PARTITION_ENABLE_MASK	(7 << EXT_CSD_BOOT_PARTITION_ENABLE_SHIFT)
# define EXT_CSD_PARTITION_ACCESS_MASK		7

#define EXT_CSD_BUS_WIDTH		183	/* R/W */
# define EXT_CSD_BUS_WIDTH_1		0	/* Card is in 1 bit mode */
# define EXT_CSD_BUS_WIDTH_4		1	/* Card is in 4 bit mode */
# define EXT_CSD_BUS_WIDTH_8		2	/* Card is in 8 bit mode */
# define EXT_CSD_DDR_BUS_WIDTH_4	5	/* Card is in 4 bit DDR mode */
# define EXT_CSD_DDR_BUS_WIDTH_8	6	/* Card is in 8 bit DDR mode */

#define EXT_CSD_HS_TIMING		185	/* R/W */
# define EXT_CSD_TIMING_BC		0	/*Backwards compatility*/
# define EXT_CSD_TIMING_HS		1	/*High speed*/
# define EXT_CSD_TIMING_HS200		2	/*HS200*/

#define EXT_CSD_REV			192	/* RO */
#define EXT_CSD_CARD_TYPE		196	/* RO */
#define EXT_CSD_SEC_CNT			212	/* RO, 4 bytes */
#define EXT_CSD_HC_ERASE_GRP_SIZE	224	/* RO */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL		(1 << 0)
#define EXT_CSD_CMD_SET_SECURE		(1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE	(1 << 2)

#define EXT_CSD_CARD_TYPE_26	(1 << 0)	/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52	(1 << 1)	/* Card can run at 52MHz */

#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */

#define R1_ILLEGAL_COMMAND		(1 << 22)
#define R1_APP_CMD			(1 << 5)

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| \
			MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMCPART_NOAVAILABLE	(0xff)
#define PART_SUPPORT		(0x1)


/* sdhci.h */
#define SDHCI_DMA_ADDRESS	0x00

#define SDHCI_BLOCK_SIZE	0x04
#define  SDHCI_MAKE_BLKSZ(dma, blksz) (((dma & 0x7) << 12) | (blksz & 0xFFF))

#define SDHCI_BLOCK_COUNT	0x06

#define SDHCI_ARGUMENT		0x08

#define SDHCI_TRANSFER_MODE	0x0C
#define  SDHCI_TRNS_DMA		0x01
#define  SDHCI_TRNS_BLK_CNT_EN	0x02
#define  SDHCI_TRNS_ACMD12	0x04
#define  SDHCI_TRNS_READ	0x10
#define  SDHCI_TRNS_MULTI	0x20

#define SDHCI_COMMAND		0x0E
#define  SDHCI_CMD_RESP_MASK	0x03
#define  SDHCI_CMD_CRC		0x08
#define  SDHCI_CMD_INDEX	0x10
#define  SDHCI_CMD_DATA		0x20
#define  SDHCI_CMD_ABORTCMD	0xC0

#define  SDHCI_CMD_RESP_NONE	0x00
#define  SDHCI_CMD_RESP_LONG	0x01
#define  SDHCI_CMD_RESP_SHORT	0x02
#define  SDHCI_CMD_RESP_SHORT_BUSY 0x03

#define SDHCI_MAKE_CMD(c, f) (((c & 0xff) << 8) | (f & 0xff))
#define SDHCI_GET_CMD(c) ((c>>8) & 0x3f)

#define SDHCI_RESPONSE		0x10

#define SDHCI_BUFFER		0x20

#define SDHCI_PRESENT_STATE	0x24
#define  SDHCI_CMD_INHIBIT	0x00000001
#define  SDHCI_DATA_INHIBIT	0x00000002
#define  SDHCI_DOING_WRITE	0x00000100
#define  SDHCI_DOING_READ	0x00000200
#define  SDHCI_SPACE_AVAILABLE	0x00000400
#define  SDHCI_DATA_AVAILABLE	0x00000800
#define  SDHCI_CARD_PRESENT	0x00010000
#define  SDHCI_WRITE_PROTECT	0x00080000

#define SDHCI_HOST_CONTROL	0x28
#define  SDHCI_CTRL_LED		0x01
#define  SDHCI_CTRL_4BITBUS	0x02
#define  SDHCI_CTRL_HISPD	0x04
#define  SDHCI_CTRL_DMA_MASK	0x18
#define   SDHCI_CTRL_SDMA	0x00
#define   SDHCI_CTRL_ADMA1	0x08
#define   SDHCI_CTRL_ADMA32	0x10
#define   SDHCI_CTRL_ADMA64	0x18
#define   SDHCI_CTRL_8BITBUS	0x20

#define	SDHCI_HOST_CONTROL2	0x3e
#define	SDHCI_CTRL_UHS_MASK	0x0007
#define	SDHCI_CTRL_UHS_SDR12	0x0000
#define	SDHCI_CTRL_UHS_SDR25	0x0001
#define	SDHCI_CTRL_UHS_SDR50	0x0002
#define	SDHCI_CTRL_UHS_SDR104	0x0003
#define	SDHCI_CTRL_UHS_DDR50	0x0004
#define	SDHCI_CTRL_HS_SDR200	0x0005 /* reserved value in SDIO spec */

#define SDHCI_POWER_CONTROL	0x29
#define  SDHCI_POWER_ON		0x01
#define  SDHCI_POWER_180	0x0A
#define  SDHCI_POWER_300	0x0C
#define  SDHCI_POWER_330	0x0E

#define SDHCI_BLOCK_GAP_CONTROL	0x2A

#define SDHCI_WAKE_UP_CONTROL	0x2B
#define  SDHCI_WAKE_ON_INT	0x01
#define  SDHCI_WAKE_ON_INSERT	0x02
#define  SDHCI_WAKE_ON_REMOVE	0x04

#define SDHCI_CLOCK_CONTROL	0x2C
#define  SDHCI_DIVIDER_SHIFT	8
#define  SDHCI_DIVIDER_HI_SHIFT	6
#define  SDHCI_DIV_MASK	0xFF
#define  SDHCI_DIV_MASK_LEN	8
#define  SDHCI_DIV_HI_MASK	0x300
#define  SDHCI_CLOCK_CARD_EN	0x0004
#define  SDHCI_CLOCK_INT_STABLE	0x0002
#define  SDHCI_CLOCK_INT_EN	0x0001

#define SDHCI_TIMEOUT_CONTROL	0x2E

#define SDHCI_SOFTWARE_RESET	0x2F
#define  SDHCI_RESET_ALL	0x01
#define  SDHCI_RESET_CMD	0x02
#define  SDHCI_RESET_DATA	0x04

#define SDHCI_INT_STATUS	0x30
#define SDHCI_INT_ENABLE	0x34
#define SDHCI_SIGNAL_ENABLE	0x38
#define  SDHCI_INT_RESPONSE	0x00000001
#define  SDHCI_INT_DATA_END	0x00000002
#define  SDHCI_INT_DMA_END	0x00000008
#define  SDHCI_INT_SPACE_AVAIL	0x00000010
#define  SDHCI_INT_DATA_AVAIL	0x00000020
#define  SDHCI_INT_CARD_INSERT	0x00000040
#define  SDHCI_INT_CARD_REMOVE	0x00000080
#define  SDHCI_INT_CARD_INT	0x00000100
#define  SDHCI_INT_ERROR	0x00008000
#define  SDHCI_INT_TIMEOUT	0x00010000
#define  SDHCI_INT_CRC		0x00020000
#define  SDHCI_INT_END_BIT	0x00040000
#define  SDHCI_INT_INDEX	0x00080000
#define  SDHCI_INT_DATA_TIMEOUT	0x00100000
#define  SDHCI_INT_DATA_CRC	0x00200000
#define  SDHCI_INT_DATA_END_BIT	0x00400000
#define  SDHCI_INT_BUS_POWER	0x00800000
#define  SDHCI_INT_ACMD12ERR	0x01000000
#define  SDHCI_INT_ADMA_ERROR	0x02000000

#define  SDHCI_INT_NORMAL_MASK	0x00007FFF
#define  SDHCI_INT_ERROR_MASK	0xFFFF8000

#define  SDHCI_INT_CMD_MASK	(SDHCI_INT_RESPONSE | SDHCI_INT_TIMEOUT | \
		SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)
#define  SDHCI_INT_DATA_MASK	(SDHCI_INT_DATA_END | SDHCI_INT_DMA_END | \
		SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL | \
		SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC | \
		SDHCI_INT_DATA_END_BIT | SDHCI_INT_ADMA_ERROR)
#define SDHCI_INT_ALL_MASK	((unsigned int)-1)

#define SDHCI_ACMD12_ERR	0x3C

/* 3E-3F reserved */

#define SDHCI_CAPABILITIES	0x40
#define  SDHCI_TIMEOUT_CLK_MASK	0x0000003F
#define  SDHCI_TIMEOUT_CLK_SHIFT 0
#define  SDHCI_TIMEOUT_CLK_UNIT	0x00000080
#define  SDHCI_CLOCK_BASE_MASK	0x00003F00
#define  SDHCI_CLOCK_V3_BASE_MASK	0x0000FF00
#define  SDHCI_CLOCK_BASE_SHIFT	8
#define  SDHCI_MAX_BLOCK_MASK	0x00030000
#define  SDHCI_MAX_BLOCK_SHIFT  16
#define  SDHCI_CAN_DO_8BIT	0x00040000
#define  SDHCI_CAN_DO_ADMA2	0x00080000
#define  SDHCI_CAN_DO_ADMA1	0x00100000
#define  SDHCI_CAN_DO_HISPD	0x00200000
#define  SDHCI_CAN_DO_SDMA	0x00400000
#define  SDHCI_CAN_VDD_330	0x01000000
#define  SDHCI_CAN_VDD_300	0x02000000
#define  SDHCI_CAN_VDD_180	0x04000000
#define  SDHCI_CAN_64BIT	0x10000000

#define SDHCI_CAPABILITIES_1	0x44

#define SDHCI_MAX_CURRENT	0x48

/* 4C-4F reserved for more max current */

#define SDHCI_SET_ACMD12_ERROR	0x50
#define SDHCI_SET_INT_ERROR	0x52

#define SDHCI_ADMA_ERROR	0x54

/* 55-57 reserved */

#define SDHCI_ADMA_ADDRESS	0x58

/* 60-FB reserved */

#define SDHCI_SLOT_INT_STATUS	0xFC

#define SDHCI_HOST_VERSION	0xFE
#define  SDHCI_VENDOR_VER_MASK	0xFF00
#define  SDHCI_VENDOR_VER_SHIFT	8
#define  SDHCI_SPEC_VER_MASK	0x00FF
#define  SDHCI_SPEC_VER_SHIFT	0
#define   SDHCI_SPEC_100	0
#define   SDHCI_SPEC_200	1
#define   SDHCI_SPEC_300	2

/*
 * End of controller registers.
 */

#define SDHCI_MAX_DIV_SPEC_200	256
#define SDHCI_MAX_DIV_SPEC_300	2046

/*
 * quirks
 */
#define SDHCI_QUIRK_32BIT_DMA_ADDR	(1 << 0)
#define SDHCI_QUIRK_REG32_RW		(1 << 1)

/* to make gcc happy */
//struct sdhci_host;

/*
 * Host SDMA buffer boundary. Valid values from 4K to 512K in powers of 2.
 */
#define SDHCI_DEFAULT_BOUNDARY_SIZE	(512 * 1024)
#define SDHCI_DEFAULT_BOUNDARY_ARG	(7)


#define sdhci_writel(v, a) do { 	\
	mmio_write_32(SDHC_REG(a), v);	\
} while(0)
#define sdhci_writew(v, a) do { 	\
	mmio_write_16(SDHC_REG(a), v);	\
} while(0)
#define sdhci_writeb(v, a) do { 	\
	mmio_write_8(SDHC_REG(a), v);	\
} while(0)

#define sdhci_readl(a) (mmio_read_32(SDHC_REG(a)))
#define sdhci_readw(a) (mmio_read_16(SDHC_REG(a)))
#define sdhci_readb(a) (mmio_read_8(SDHC_REG(a)))

struct emmc_cmd {
	unsigned int cmdidx;
	unsigned int resp_type;
	unsigned int cmdarg;
	unsigned int response[4];
	unsigned int flags;
};

struct emmc_data {
	char *data;
	unsigned int flags;
	unsigned int blocks;
	unsigned int blocksize;
	unsigned int get_extcsd;
	unsigned int need_skip;
	unsigned int skip_size;
	unsigned int skip_done;
	unsigned int rdblks;
};

static unsigned int extcsd_part_config = -1;
static int sdhci_initialized = 0;

static void emmc_transfer_pio(struct emmc_data *data)
{
	int i;
	int data_length;
	int last_block = 0;
	char *offs;
	if (data->data == NULL && data->get_extcsd == 1) {
	/*
	* In preboot, we don't have enough space for full 
	* extcsd, so this is only for get some particular 
 	* value that we interested.
 	*/
#ifdef DEBUG_EMMC
		puts("\n");
#endif

		int temp;
		char *pstr = (char *)&temp;
		for(i=0; i<EMMC_BLOCK_SIZE; i+=4) {
			temp = sdhci_readl(SDHCI_BUFFER);
#ifdef DEBUG_EMMC
			printf("%03d : 0x%08x\n", i, temp);
#endif
			if (i<179 && (179-i)<4) {
				pstr = (char *)&temp;
				pstr += (179-i);
				extcsd_part_config = (*pstr);
				INFO("temp = 0x%x\n", temp);
			}
		}

		return;
	}
	
	if ((data->rdblks + 1) == data->blocks) {
		last_block = 1;
	}

	if (data->need_skip && data->skip_done == 0) {

		for(i = 0; i < data->skip_size; i += 4) {
			sdhci_readl(SDHCI_BUFFER);
		}
		data_length = (data->blocksize - data->skip_size);
		data->skip_done = 1;

	} else {

		data_length = data->blocksize;
	}

	if (data->need_skip && last_block && data->skip_done) {

		data_length = data->skip_size;
	}

	for (i = 0; i < data_length; i += 4) {
		offs = data->data + i;
		if (data->flags == MMC_DATA_READ)
			*(uint32_t *)offs = sdhci_readl(SDHCI_BUFFER);
		else
			sdhci_writel(*(uint32_t *)offs, SDHCI_BUFFER);
	}

	if (last_block) {
		for (i=0; i< (data->blocksize - data->skip_size); i+=4) {
			sdhci_readl(SDHCI_BUFFER);
		}
		data->need_skip = 0;
		data->skip_done = 0;
		data->skip_size = 0;
	}
	data->data += data_length;

}
static void emmc_cmd_done(struct emmc_cmd *cmd)
{
	int i;
	if (cmd->resp_type & MMC_RSP_136) {
		/* CRC is stripped so we need to do some shifting. */
		for (i = 0; i < 4; i++) {
			cmd->response[i] = sdhci_readl(
					SDHCI_RESPONSE + (3-i)*4) << 8;
			if (i != 3)
				cmd->response[i] |= sdhci_readb(
						SDHCI_RESPONSE + (3-i)*4-1);
		}
	} else {
		cmd->response[0] = sdhci_readl(SDHCI_RESPONSE);
	}
}
static int __emmc_send_cmd(struct emmc_cmd *cmd, struct emmc_data *data)
{
	unsigned int mode, mask, stat, rdy;
	int ret = 0;
	
	mask = SDHCI_INT_RESPONSE;
	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		cmd->flags = SDHCI_CMD_RESP_NONE;
	else if (cmd->resp_type & MMC_RSP_136)
		cmd->flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->resp_type & MMC_RSP_BUSY) {
		cmd->flags = SDHCI_CMD_RESP_SHORT_BUSY;
		mask |= SDHCI_INT_DATA_END;
	} else
		cmd->flags = SDHCI_CMD_RESP_SHORT;

	if (cmd->resp_type & MMC_RSP_CRC)
		cmd->flags |= SDHCI_CMD_CRC;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		cmd->flags |= SDHCI_CMD_INDEX;
	if (data)
		cmd->flags |= SDHCI_CMD_DATA;

	
	/*Set Transfer mode regarding to data flag*/
	if (data != 0) {
		sdhci_writeb(0xe, SDHCI_TIMEOUT_CONTROL);
		mode = SDHCI_TRNS_BLK_CNT_EN;
		if (data->blocks > 1)
			mode |= SDHCI_TRNS_MULTI;

		if (data->flags == MMC_DATA_READ)
			mode |= SDHCI_TRNS_READ;

		sdhci_writew(SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,
				data->blocksize),
				SDHCI_BLOCK_SIZE);
		sdhci_writew(data->blocks, SDHCI_BLOCK_COUNT);
		sdhci_writew(mode, SDHCI_TRANSFER_MODE);
	}
	sdhci_writel(cmd->cmdarg, SDHCI_ARGUMENT);
	sdhci_writew(SDHCI_MAKE_CMD(cmd->cmdidx, cmd->flags), SDHCI_COMMAND);
	
	do {
		stat = sdhci_readl(SDHCI_INT_STATUS);
#ifdef DEBUG_EMMC
		printf("\t sdhci_int_stat = 0x%08x\n", stat);
#endif
		if (stat & SDHCI_INT_ERROR)
			break;
	} while ((stat & mask) != mask);

	if ((stat & (SDHCI_INT_ERROR | mask)) == mask) {
		emmc_cmd_done(cmd);
		sdhci_writel(mask, SDHCI_INT_STATUS);
	} else {
#ifdef DEBUG_EMMC
		ERROR("Got error interrupt, stat = 0x%x\n", stat);
#endif
		ret = -1;
	}

	if (!ret && data) {
		/*
		* Do data transfer 
		*/
		rdy = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL;
		mask = SDHCI_DATA_AVAILABLE | SDHCI_SPACE_AVAILABLE;
		data->rdblks = 0;
		
		do {
		
			stat = sdhci_readl(SDHCI_INT_STATUS);
			if (stat & SDHCI_INT_ERROR) {
#ifdef DEBUG_EMMC
				ERROR("Got error interrupt(data) stat= 0x%x\n", stat);
#endif
				ret = -1;
				break;
			}
			if (stat & rdy) {
				if (!(sdhci_readl(SDHCI_PRESENT_STATE) & mask))
					continue;
				sdhci_writel(rdy, SDHCI_INT_STATUS);
				emmc_transfer_pio(data);
				data->rdblks++; 
				if (data->rdblks >= data->blocks)
					break;
			}

		} while (!(stat & SDHCI_INT_DATA_END));
		
	}
	stat = sdhci_readl(SDHCI_INT_STATUS);
	sdhci_writel(SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);

	if (!ret) {
		return 0;
	}

	sdhci_writeb(SDHCI_RESET_CMD, SDHCI_SOFTWARE_RESET);
	while (sdhci_readb(SDHCI_SOFTWARE_RESET) & SDHCI_RESET_CMD);

	sdhci_writeb(SDHCI_RESET_DATA, SDHCI_SOFTWARE_RESET);
	while (sdhci_readb(SDHCI_SOFTWARE_RESET) & SDHCI_RESET_DATA);

	return -1;
}
static int emmc_send_cmd(struct emmc_cmd *cmd, struct emmc_data *data)
{
	int ret = -1;

#ifdef DEBUG_EMMC	
	INFO("CMD: %d\n", cmd->cmdidx);
#endif
	ret = __emmc_send_cmd(cmd, data);

	return ret;
}
static int emmc_switch(unsigned char index, unsigned char value)
{
	int ret = -1;
	struct emmc_cmd cmd;
	memset(&cmd, 0, sizeof(struct emmc_cmd));

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
				(index<< 16) |
				(value << 8)| 
				EXT_CSD_CMD_SET_NORMAL;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.flags = 0;
	ret = emmc_send_cmd(&cmd, NULL);
	if (ret)
		goto error;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 1<<16;
	cmd.flags = 0;
	do {
		ret = emmc_send_cmd(&cmd, NULL);
		if (!ret) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;
			else if (cmd.response[0] & MMC_STATUS_MASK) {
				ERROR("Status Error: 0x%x\n",
					cmd.response[0]);
				ret = -1;
				break;
			}
		}

	} while (1);
error:
	return ret;
}

static int emmc_switch_part(int partno)
{
#define BOOT_PART(_conf) 					\
	((_conf & EXT_CSD_BOOT_PARTITION_ENABLE_MASK) >>	\
	EXT_CSD_BOOT_PARTITION_ENABLE_SHIFT)
#define PART_ACCESS(_conf)					\
	(_conf & EXT_CSD_PARTITION_ACCESS_MASK)
#define MK_PART_CONFIG(_conf, _acc)				\
	((_conf & (~EXT_CSD_PARTITION_ACCESS_MASK)) |		\
	(_acc & EXT_CSD_PARTITION_ACCESS_MASK))
	int ret = 0;
	unsigned char access = 0xff;
	if (extcsd_part_config == -1){
		struct emmc_cmd cmd;
		struct emmc_data data;
		memset(&cmd, 0, sizeof(struct emmc_cmd));
		memset(&data, 0, sizeof(struct emmc_data));
		cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1;
		cmd.flags = 0;
		data.data = NULL;
		data.blocks = 1;
		data.blocksize = EMMC_BLOCK_SIZE;
		data.flags = MMC_DATA_READ;
		data.get_extcsd = 1;
		ret = emmc_send_cmd(&cmd, &data);
		data.get_extcsd = 0;
		if (ret) {
#ifdef DEBUG_EMMC
			ERROR("failed to get ext_csd, ret %d\n", ret);
#endif
			goto error;
		}
	}

#ifdef DEBUG_EMMC
	INFO("part_config 0x%x, partno %d\n", extcsd_part_config, partno);
#endif
	if (EMMC_PART_BOOT == partno) {
		if (BOOT_PART(extcsd_part_config)==1 || BOOT_PART(extcsd_part_config)==2) {
			if (PART_ACCESS(extcsd_part_config) != BOOT_PART(extcsd_part_config))
				access = BOOT_PART(extcsd_part_config);
		} else {
#ifdef DEBUG_EMMC
			ERROR("failed to fixup part, part_confg 0x%x\n", extcsd_part_config);
#endif
			ret = -1;
			goto error;
		}
	} else if (PART_ACCESS(extcsd_part_config) != partno){
		access = partno;
	}

	if (access != 0xff) {
		unsigned char tmp = MK_PART_CONFIG(extcsd_part_config, access);
		ret = emmc_switch(EXT_CSD_PART_CONF, tmp);
		if (0 == ret)
			extcsd_part_config = tmp;
	}
error:
	return ret;
}

static int emmc_deinit(void)
{
	sdhci_writeb(SDHCI_RESET_ALL, SDHCI_SOFTWARE_RESET);
	while (sdhci_readb(SDHCI_SOFTWARE_RESET) & SDHCI_RESET_ALL);
	return 0;
}

static int emmc_init(void)
{
	struct emmc_cmd cmd;

	int clk, ret, ctrl, tapdelay;

#if (SD_BOOTDEV == SD_FLASH_NOR)
	/*
	 * Wait for SDHCI controller ready to send command.
	 * eMMC boot mode do not need this, due to Boot Rom done this.
	 */ 
	while((sdhci_readl(SDHCI_PRESENT_STATE)& SDHCI_CARD_PRESENT) == 0);
#endif

	sd_soc_pinshare_init_for_mmc(0);

	/* Reset HC */
	sdhci_writeb(SDHCI_RESET_ALL, SDHCI_SOFTWARE_RESET);
	while (sdhci_readb(SDHCI_SOFTWARE_RESET) & SDHCI_RESET_ALL);

	/* Enable interrupt */	
	sdhci_writel(SDHCI_INT_ALL_MASK, SDHCI_INT_ENABLE);
	sdhci_writel(SDHCI_INT_ALL_MASK, SDHCI_SIGNAL_ENABLE);
	
	/* Enable power */
	sdhci_writeb(SDHCI_POWER_330|SDHCI_POWER_ON, \
					SDHCI_POWER_CONTROL);

	/* Set clock to minimum frequence */
	sdhci_writew(0, SDHCI_CLOCK_CONTROL);
	clk = 0xffc0;
	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(clk, SDHCI_CLOCK_CONTROL);	
	while (!(sdhci_readw(SDHCI_CLOCK_CONTROL) & \
					SDHCI_CLOCK_INT_STABLE));
	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(clk, SDHCI_CLOCK_CONTROL);


	/* Issue cmd0, Reset card */
	memset(&cmd, 0, sizeof(struct emmc_cmd));
	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;
	ret = emmc_send_cmd(&cmd, NULL);
	if (ret)
		goto error;
	
	/* Issue cmd1, card goto ready status */
	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.cmdarg = 0x40FF8080;
		cmd.resp_type = MMC_RSP_R3;
		cmd.flags = 0;
		ret = emmc_send_cmd(&cmd, NULL);

	} while (!(cmd.response[0] & OCR_BUSY));
	/* Issue cmd2 broadcast SEND_ALL_CID 
	 * card goto identification mode
	 */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R2;
	cmd.flags = 0;
	ret = emmc_send_cmd(&cmd, NULL);
	if (ret)
		goto error;
	
	/* Issue cmd3 SET_RELATIVE_ADDR,
 	 * card enter standby state. 
 	 */
	cmd.cmdidx = MMC_CMD_SET_RELATIVE_ADDR;
	cmd.cmdarg = 1<<16;
	cmd.resp_type = MMC_RSP_R6;
	cmd.flags = 0;
	ret = emmc_send_cmd(&cmd, NULL);
	if (ret)
		goto error;

	/* Issue cmd7 SELECT_CARD
 	*  Put card into transfer mode.
 	*/
	cmd.cmdidx = MMC_CMD_SELECT_CARD;
	cmd.cmdarg = 1<<16;
	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;
	ret = emmc_send_cmd(&cmd, NULL);
	if (ret)
		goto error;
	
	/* Switch card to HS mode */
	ret = emmc_switch(EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);
	if (ret)
		goto error;

	/* Switch card to 8bit DDR bus mode */
	ret = emmc_switch(EXT_CSD_BUS_WIDTH, EXT_CSD_DDR_BUS_WIDTH_8);
	if (ret)
		goto error;


	/* Set SDHC to 8bit DDR bus mode */
	ctrl = sdhci_readb(SDHCI_HOST_CONTROL);
	ctrl |= (SDHCI_CTRL_8BITBUS | SDHCI_CTRL_HISPD);
	sdhci_writeb(ctrl, SDHCI_HOST_CONTROL);

	ctrl = sdhci_readw(SDHCI_HOST_CONTROL2);
	ctrl &= ~SDHCI_CTRL_UHS_MASK;
	ctrl |= SDHCI_CTRL_UHS_DDR50;
	sdhci_writew(ctrl, SDHCI_HOST_CONTROL2);

	/* Set SDHC clock to 50Mhz */
	sdhci_writew(0, SDHCI_CLOCK_CONTROL);
	clk = 0x0200;
	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(clk, SDHCI_CLOCK_CONTROL);	
	while (!(sdhci_readw(SDHCI_CLOCK_CONTROL) & \
					SDHCI_CLOCK_INT_STABLE));
	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(clk, SDHCI_CLOCK_CONTROL);

	/*
	 * Set SDHC tap delay in DDR50 mode
	 * Derive tap delay value from OTP as ROM does
	 */
	tapdelay = fuse_read_field(gpfc_0, emmc_ddr_timing);
	sdhci_writel((sdhci_readl(0x400) & ~0x1fff) | 0x1000 | (tapdelay << 8), 0x400);

	sdhci_initialized = 1;

error:
	return ret;
	
}

static int emmc_read(int ofs, uintptr_t buf, size_t size)
{
	struct emmc_cmd cmd;
	struct emmc_data data;
	unsigned int blkcnt = (size / EMMC_BLOCK_SIZE);

	memset(&cmd, 0, sizeof(struct emmc_cmd));
	memset(&data, 0, sizeof(struct emmc_data));
	if (ofs & EMMC_BLOCK_MASK)
		blkcnt += 1;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else 
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	cmd.cmdarg = (ofs / EMMC_BLOCK_SIZE);
	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.data = (char*)buf;
	data.blocks = blkcnt;
	data.blocksize = EMMC_BLOCK_SIZE;
	data.flags = MMC_DATA_READ;
	data.skip_size = (ofs & EMMC_BLOCK_MASK);
	if (data.skip_size) {
		data.need_skip = 1;
		data.skip_done = 0;
	} else {
		data.need_skip = 0;
		data.skip_done = 0;
	}

#ifdef DEBUG_EMMC
	INFO("emmc read: start 0x%x buf %p size 0x%lx\n", ofs, (void*)buf, size);
#endif
	if (emmc_send_cmd(&cmd, &data)) {
		goto error;
	}

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (emmc_send_cmd(&cmd, NULL)) {
			ERROR("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	return size;
error:
	return 0;
}

int sd_emmc_init(void)
{
	return emmc_init();
}

void sd_emmc_deinit(void)
{
	emmc_deinit();
}

/*
 * read data from emmc user partition
 * In:
 *	lba	- least block aligned address
 *	buf	- pointer of buffer for out data
 *	size	- bytes of read data, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t emmc_read_blocks(int lba, uintptr_t buf, size_t size)
{
	if (!emmc_switch_part(EMMC_PART_USER))
		return emmc_read((lba * EMMC_BLOCK_SIZE), buf, size);
	else
		return 0;
}

size_t emmc_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	//TODOs...
	return 0;
}

/*
 * read data from present selected boot partition (boot0 or boo1)
 * In:
 *	lba	- least block aligned address
 *	buf	- pointer of buffer for out data
 *	size	- bytes of read data, block size aligned
 * Out:
 *	return bytes of read data on success
 */
size_t emmc_boot_read_blocks(int lba, uintptr_t buf, size_t size)
{
	if (!emmc_switch_part(EMMC_PART_BOOT))
		return emmc_read((lba * EMMC_BLOCK_SIZE), buf, size);
	else
		return 0;
}

size_t emmc_boot_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	//TODOs...
	return 0;
}

size_t emmc_rpmb_read_blocks(int lba, uintptr_t buf, size_t size)
{
	if (!emmc_switch_part(EMMC_PART_RPMB))
		return emmc_read((lba * EMMC_BLOCK_SIZE), buf, size);
	else
		return 0;
}

size_t emmc_rpmb_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	//TODOs...
	return 0;
}
