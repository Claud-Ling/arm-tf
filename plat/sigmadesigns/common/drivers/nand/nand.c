#include <mmio.h>
#include <stdio.h>
#include <bl_common.h>
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <delay_timer.h>
#include <platform_def.h>
#include <sd_private.h>
#include <sd_otp.h>

/* Logger helpers */
#if LOG_LEVEL >= LOG_LEVEL_NOTICE
# define NOTICE_DUMP(...) tf_printf(__VA_ARGS__)
#else
# define NOTICE_DUMP(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
# define ERROR_DUMP(...) tf_printf(__VA_ARGS__)
#else
# define ERROR_DUMP(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
#define WARN_DUMP(...) tf_printf(__VA_ARGS__)
#else
# define WARN_DUMP(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
# define INFO_DUMP(...) tf_printf(__VA_ARGS__)
#else
# define INFO_DUMP(...)
#endif

/*
 * TSB register file base address
 */
#define NAND_BASE	SD_TSB_BASE
#define NAND_IO		(SD_TSB1_BASE + 0x0000)
#define NAND_CLE	(SD_TSB1_BASE + 0x2000)  //Fra13
#define NAND_ALE	(SD_TSB1_BASE + 0x4000)  //Fra14

#define BASE                 (NAND_BASE)  /* here goes nand reg base from top config */

#define NAND_BOOTROM_R_TIMG  (BASE + 0x00000000) /* boot rom read timing control register */
#define NAND_FCR             (BASE + 0x00000004) /* flash rom control register */
#define NAND_DCR             (BASE + 0x00000008) /* data control register */
#define NAND_TCR             (BASE + 0x0000000c) /* timing control register */
#define NAND_SYN_MODE0       (BASE + 0x00000010) /* sync mode control 0 register */
#define NAND_SYN_MODE1       (BASE + 0x00000014) /* sync mode control 1 register */
#define NAND_SPI_MODE_CMD    (BASE + 0x00000018) /* sync mode control 2 register */
#define NAND_OP_MODES        (BASE + 0x0000001c) /* */

#define SHIFT_WORK_MODE(a)   (((a) & 0x0000000f) << 26)
#define SHIFT_RD_FIN_SW(a)   (((a) & 0x00000001) << 25)
#define SHIFT_WR_FIN_SW(a)   (((a) & 0x00000001) << 24)
#define SHIFT_RD_BACK_SW(a)  (((a) & 0x000000ff) << 15)
#define SHIFT_CS_HI_TIME(a)  (((a) & 0x000000ff) << 8 )
#define SHIFT_SPI_TX_SW(a)   (((a) & 0x000000ff)      )

#define NAND_SPI_DLY_CTRL    (BASE + 0x0000020) /* SPI mode clock delay control register */
#define NAND_SPI_TIMG_CTRL0  (BASE + 0x0000024) /* SPI mode timing control register 0 */
#define NAND_SPI_TIMG_CTRL1  (BASE + 0x0000028) /* SPI mode timing control register 1 */
#define NAND_BASE_ADDR0      (BASE + 0x000002c) /* base address register 0 */
#define NAND_BASE_ADDR1      (BASE + 0x0000030) /* base address register 1 */
#define NAND_BURST_TIMG      (BASE + 0x0000034) /* flashrom burst timing control register */
#define NAND_BASE_ADDR2      (BASE + 0x0000038) /* base address register 2 */
#define NAND_BASE_ADDR3      (BASE + 0x000003c) /* base address register 3 */
#define NAND_BASE_ADDR4      (BASE + 0x0000040) /* base address register 4 */

#define NAND_ONE_DATA_ECC0   (BASE + 0x00000044) /* onenand flash data spcae ecc registe 0 */
#define NAND_ONE_DATA_ECC1   (BASE + 0x00000048) /* onenand flash data spcae ecc registe 1 */
#define NAND_ONE_DATA_ECC2   (BASE + 0x0000004c) /* onenand flash data spcae ecc registe 2 */
#define NAND_ONE_SPR_ECC0    (BASE + 0x00000050) /* onenand flash spare space ecc register 0 */
#define NAND_ONE_SPR_ECC1    (BASE + 0x00000054) /* onenand flash spare space ecc register 1 */

#define NAND_CTRL            (BASE + 0x00000058) /* nand flash control register */
#define NAND_TIMG            (BASE + 0x0000005c) /* nand flash timing register */

/*SX7 new nand controller*/
#define NAND_CCR             (BASE + 0x000000A0) /* Command list control Register */

/* ---------------------------------------------------------------------
   address hole in the middle
   ---------------------------------------------------------------------*/

#define NAND_MLC_ECC_CTRL            (BASE + 0x00000070) /* MLC ECC control register */
#define NAND_MLC_ESS_STATUS          (BASE + 0x00000074) /* MLC ECC status register */
#define NAND_MLC_ECC_DATA            (BASE + 0x00000078) /* MLC ECC data register */
#define NAND_MLC_ECC_ENC_DATA1       (BASE + 0x0000007c) /* MLC ECC encoder data register 1*/
#define NAND_MLC_ECC_DEC_ERR_POS0    (BASE + 0x00000080) /* MLC ECC decoder error position egister 0*/
#define NAND_MLC_ECC_DEC_ERR_POS1    (BASE + 0x00000084) /* MLC ECC decoder error position egister 1*/

#define NAND_FDMA_SRC_ADDR           (0x00000088) /*FDMA SOURCE ADDR*/
#define NAND_FDMA_DEST_ADDR          (0x0000008c) /*FDMA DESTINATION ADDR*/
#define NAND_FDMA_INT_ADDR           (0x00000090) /*FDMA INTERRUPT ADDR*/
#define NAND_FDMA_LEN_ADDR           (0x00000094) /*FDMA LENGTH ADDR */

#define NAND_SW_RB_DETECT           (0x00000098)  /*software detect NAND ready*/
/* ---------------------------------------------------------------------
 *    ECC related register
 * ---------------------------------------------------------------------*/
#define NAND_ECC_CTRL                (BASE + 0x00000200) /* ECC control register */


#define MLC_ECC_REG_CONTROL     (BASE + 0x00000070) /* MLC ECC control register */
#define MLC_ECC_REG_STATUS      (BASE + 0x00000074) /* MLC ECC status register */

#define MLC_ECC_CONTROL_DECODER_CLEAR    0x08
#define MLC_ECC_CONTROL_DECODER_START    0x04
#define MLC_ECC_CONTROL_ENCODER_CLEAR    0x02
#define MLC_ECC_CONTROL_ENCODER_START    0x01
#define REG_BCH_ECC_BASE                (BASE + 0x00000200) /* ECC control register */
#define REG_BCH_ECC_MAP(x)              (REG_BCH_ECC_BASE + x)

#define REG_BCH_ECC_CONTROL             REG_BCH_ECC_MAP(0x00)
#define REG_BCH_ECC_STATUS              REG_BCH_ECC_MAP(0x04)

#define REG_BCH_ECC_DATA_0              REG_BCH_ECC_MAP(0x10)
#define REG_BCH_ECC_DATA_1              REG_BCH_ECC_MAP(0x14)
#define REG_BCH_ECC_DATA_2              REG_BCH_ECC_MAP(0x18)
#define REG_BCH_ECC_DATA_3              REG_BCH_ECC_MAP(0x1C)
#define REG_BCH_ECC_DATA_4              REG_BCH_ECC_MAP(0x20)
#define REG_BCH_ECC_DATA_5              REG_BCH_ECC_MAP(0x24)
#define REG_BCH_ECC_DATA_6              REG_BCH_ECC_MAP(0x28)
#define REG_BCH_ECC_DATA_7              REG_BCH_ECC_MAP(0x2C)
#define REG_BCH_ECC_DATA_8              REG_BCH_ECC_MAP(0x30)
#define REG_BCH_ECC_DATA_9              REG_BCH_ECC_MAP(0x34)
#define REG_BCH_ECC_DATA_a              REG_BCH_ECC_MAP(0x38)
#define REG_BCH_ECC_DATA_b              REG_BCH_ECC_MAP(0x3C)

#define REG_BCH_ECC_CORRECTION_0        REG_BCH_ECC_MAP(0x60)
#define REG_BCH_ECC_INT_SET_ENABLE      REG_BCH_ECC_MAP(0xDC)


/*
 * ECC_CONTROL
 */
#define ECC_CONTROL_CLEAR               (1<<8)
#define ECC_CONTROL_PARITY_MODE         (1<<6)      // O:DECODE, 1:ENCODE
#define ECC_CONTROL_MODE_OPERATION      (3<<4)      // 00:4 BITS ECC, 01:8 BITS ECC
#define ECC_CONTROL_MODE_OPERATION_4BITS      (3<<4)      // 00:4 BITS ECC, 01:8 BITS ECC
#define ECC_CONTROL_MODE_OPERATION_8BITS      (1<<4)      // 00:4 BITS ECC, 01:8 BITS ECC
#define ECC_CONTROL_PAYLOAD_SIZE        (3<<2)      // 01:512 BYTES
#define ECC_CONTROL_PAYLOAD_SIZE_512B   (1<<2)      // 01:512 BYTES
#define ECC_CONTROL_ENABLE_NFC          (1<<1)
#define ECC_CONTROL_ENABLE_CALC         (1<<0)

/*
 * ECC_STATUS
 */
#define ECC_STATUS_NUM_CORRECTIONS      (0x3f<<20)
#define ECC_STATUS_ERASED_PAGE          (1<<19)
#define ECC_STATUS_UNCORRECTABLE_ERR    (1<<18)
#define ECC_STATUS_CORRECTION_VALID     (1<<17)
#define ECC_STATUS_PARITY_VALID         (1<<16)
#define ECC_STATUS_WORD_COUNT           (0xffff<<0)

#define ECC_BUSY_WAIT_TIMEOUT 500 /* us */

#define nand_writel(a, v) do { 	\
	mmio_write_32(a, v);	\
} while(0)

#define nand_writew(a, v) do { 	\
	mmio_write_16(a, v);	\
} while(0)

#define nand_writeb(a, v) do { 	\
	mmio_write_8(a, v);	\
} while(0)

#define nand_readl(a) (mmio_read_32(a))
#define nand_readw(a) (mmio_read_16(a))
#define nand_readb(a) (mmio_read_8(a))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/*OTP access helpers*/
#define otp_flash_extra_delay()	\
	fuse_read_field(boot_rom_cfg_0,flash_extra_delay)
#define otp_nand_rb_sel()	\
	fuse_read_field(boot_rom_cfg_0,nand_rb_sel)
#define otp_nand_reset_on_boot()\
	fuse_read_field(boot_rom_cfg_2,nand_reset_en)
#define otp_nand_id_dis()	\
	fuse_read_field(fc_2,nand_id_dis)
#define otp_nand_page_size()	\
	fuse_read_field(boot_rom_cfg_2,nand_page_size)
#define otp_nand_addr_cycle()	\
	fuse_read_field(boot_rom_cfg_2,nand_addr_cycle)
#define otp_nand_ctrler_sel()	\
	fuse_read_field(fc_2,new_nand_ctrl_sel)
#define otp_nand_ecc_bits()	\
	(fuse_read_field(boot_rom_cfg_2,nand_ecc_bits) << 1)
#define otp_nand_ecc_unit()	\
	fuse_read_field(boot_rom_cfg_2,nand_ecc_unit)

enum nand_host {
        NAND_HOST_OLD = 0,
        NAND_HOST_NEW,
};

struct nand_chip {
	unsigned int page_size;
	unsigned int page_shift;

	unsigned int oob_ofs;
	unsigned int ecc_steps;		//number of ECC steps per page
	unsigned int ecc_size;		// data bytes per ECC step
	unsigned int ecc_bytes;		// ECC bytes per ECC step
	unsigned int ecc_strength;	//max number of correctible bits per ECC step
	unsigned int ecc_total;		//total number of ECC bytes per page
	unsigned int cycle;		//address size,see nand spec.

        enum nand_host	 	nand_ctrler;  //nand host controller
};

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0          0x0
#define NAND_CMD_READ1          0x1
#define NAND_CMD_RNDOUT         0x5
#define NAND_CMD_READID		0x90
#define NAND_CMD_RESET          0xff

#define NAND_CMD_READSTART      0x30
#define NAND_CMD_RNDOUTSTART    0xE0

#define min(X,Y) ((X) > (Y) ? (Y) : (X))

#define MAX_ECCPOS_ENTRIES_LARGE 680

static struct nand_chip nand;

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

static inline int ffs(int x)
{
        int r = 1;

        if (!x)
                return 0;
        if (!(x & 0xffff)) {
                x >>= 16;
                r += 16;
        }
        if (!(x & 0xff)) {
                x >>= 8;
                r += 8;
        }
        if (!(x & 0xf)) {
                x >>= 4;
                r += 4;
        }
        if (!(x & 3)) {
                x >>= 2;
                r += 2;
        }
        if (!(x & 1)) {
                x >>= 1;
                r += 1;
        }
        return r;
}

static unsigned int get_cur_page(unsigned int addr)
{
	unsigned int page;

	page = addr >> nand.page_shift;

	return page;
}

static void bch_decode_start(u8 *ecc)
{
	unsigned int i;
	u32 ecc_val;

	unsigned int ecc_unit = (nand.ecc_size >> 9) & 0x3; /* [3:2] 01-512bytes 10-1024bytes */
	unsigned int ecc_level = nand.ecc_strength & 0x1F;  /* [9:4] ecc level */

	nand_writel(REG_BCH_ECC_CONTROL, 0x3 | (ecc_unit << 2) | (ecc_level << 4));

	/* clear the ecc decoder, then start */
	nand_writel(MLC_ECC_REG_CONTROL, 0x108);
	nand_writel(MLC_ECC_REG_CONTROL, 0x104);

	for(i = 0; i < (nand.ecc_bytes + 3)/4; i++)
	{
		ecc_val = ecc[i*4] +
			+ ( ecc[i*4+1] << 8 )
			+ ( ecc[i*4+2] << 16 )
			+ ( ecc[i*4+3] << 24 );

		nand_writel((REG_BCH_ECC_DATA_0 + (i<<2)), ecc_val);
	}

	return;
}

static int bch_decode_correct(u8 *buf, u32 range_start, u32 range_end)
{
	int err = 0;
	u32 status, err_bits;
	u32 ecc_correction, offset;
	u8 *byte = buf;
	u16 *halfword = (u16 *)buf;
	unsigned long timeout = ECC_BUSY_WAIT_TIMEOUT;

	INFO("Calculate ecc...");
	do {
		if(nand_readl(REG_BCH_ECC_STATUS) & ECC_STATUS_CORRECTION_VALID)
			break;
		timeout --;
		udelay(1);
	}while (timeout > 0);

	if(!timeout) {
		ERROR("Error: NAND ecc is not ready\n");
		err = FLASH_ERR_IO;
		goto done;
	}

	status = nand_readl(REG_BCH_ECC_STATUS);
	if( status & ECC_STATUS_UNCORRECTABLE_ERR ) {
		ERROR("BCH: uncorrectable error,status 0x%x\n", status);
		err = FLASH_ERR_IO;
		goto done;
	}

	err_bits = (status & ECC_STATUS_NUM_CORRECTIONS)>>20;
	INFO_DUMP("%d bits error in the unit\n", err_bits);

	if(err_bits) {
		ecc_correction = REG_BCH_ECC_CORRECTION_0;
		do {
			status = nand_readl(ecc_correction);
			/*
			 * New controller:
			 * [27:16] is the error byte offset(8 element) within the payload
			 * payload = main area + oob
			 *
			 * Legacy controller:
			 * [27:16] is error halfword offset(16b element) within the payload
			 * payload = main area
			 */
			offset = (status >> 16) & 0xFFF;
			if( status == 0 ) {
				/* we reach the end of ecc correction */
				break;
			}

			if( NAND_HOST_OLD == nand.nand_ctrler) {
				/* byte offset -> halfword offset */
				range_start >>= 1;
				range_end >>= 1;
				if( (offset >= range_start) && (offset <= range_end) )
					halfword[offset - range_start] ^= (status & 0xffff);

			} else if( NAND_HOST_NEW == nand.nand_ctrler) {

				if( (offset >= range_start) && (offset <= range_end) )
					byte[offset - range_start] ^= (status & 0xff);
			}

			ecc_correction += 4; /* point to next register */
		} while(1);
	}

done:
	nand_writel(MLC_ECC_REG_CONTROL, 0x108);
	return err;
}

static void enable_nce(int enabled)
{
	unsigned int val;

	val = nand_readl(NAND_BOOTROM_R_TIMG);

	if(!enabled){
		nand_writel(NAND_BOOTROM_R_TIMG, val | 0x80);
	}
	else{
		nand_writel(NAND_BOOTROM_R_TIMG, val & ~0x80);
	}
}

static int nand_ready(void)
{
	int timeout = 1000;

	udelay(50 + otp_flash_extra_delay());

	while(timeout--) {
		if(otp_nand_rb_sel()) {
			if( (nand_readl(NAND_SW_RB_DETECT) & 0x01) != 0)
				break;
		}else {
			if( (nand_readl(NAND_BOOTROM_R_TIMG) & 0x40 ) !=0)
				break;
		}
		udelay(1);
	}

	if (timeout <= 0)
		ERROR("Nand R/B timeout\n");

	return 0;
}

static void nand_reset(void)
{
        enable_nce(1);
        nand_writel(NAND_CLE, NAND_CMD_RESET);
        nand_ready();
        enable_nce(0);
	udelay(10);

	return;
}

static void nand_scan_id(void)
{
	u32 id;

	enable_nce(1);

	if(otp_nand_rb_sel())
		nand_writel(NAND_SW_RB_DETECT, 0);

	nand_writeb(NAND_CLE, NAND_CMD_READID);
	nand_writeb(NAND_ALE, 0);
	nand_ready();

	/* Read entire ID string */
	id = nand_readl(NAND_IO);
	NOTICE("Nand ID: 0x%x\n",id);

	enable_nce(0);

	udelay(10);

	return;
}

static int nand_hw_init(void)
{
	nand_writel(NAND_BASE_ADDR0, 0x0d1c0000);
	nand_writel(NAND_BASE_ADDR1, 0x0d1c0000);
	nand_writel(NAND_FCR, 0x67);
	nand_writel(NAND_DCR, 0x2);
	nand_writel(NAND_CCR, 0x0);

	/*MUST clear decoder and encoder*/
	nand_writel(NAND_MLC_ECC_CTRL, 0x100);

	nand_writel(NAND_CTRL, 0x00000420);

	 /* ECC control reset */
	nand_writel(NAND_ECC_CTRL,(nand_readl(NAND_ECC_CTRL)&(~0x4)));
	nand_writel(NAND_ECC_CTRL, 0x1000);
	while((nand_readl(NAND_ECC_CTRL)&0x1000)!=0);
	nand_writel(NAND_ECC_CTRL, 0x02);

	return 0;
}

static int nand_init(void)
{
	nand_hw_init();

	if(otp_nand_reset_on_boot())
		nand_reset();
	if(!otp_nand_id_dis())
		nand_scan_id();

	/* nand specification is read from OTP */

	/* 4 bits: 0=512B 1=2K 2=4k 3=8k 4=16k*/
	if(!otp_nand_page_size())
		nand.page_size = 512;
	else
		nand.page_size  = 512 << (otp_nand_page_size() + 1);

	/* 4 bits: 0=3cycles 1=4cycles 2=5cycles*/
	nand.cycle  = otp_nand_addr_cycle() + 3;

	nand.nand_ctrler	= otp_nand_ctrler_sel()?NAND_HOST_NEW:NAND_HOST_OLD;

	nand.ecc_strength	= otp_nand_ecc_bits();
	nand.ecc_size		= otp_nand_ecc_unit()?1024:512;

	if(NAND_HOST_OLD == nand.nand_ctrler)
		nand.ecc_bytes = (14*nand.ecc_strength + 7) >> 3;
	else if(NAND_HOST_NEW == nand.nand_ctrler)
		nand.ecc_bytes = (15*nand.ecc_strength + 7) >> 3;

	nand.ecc_steps		= nand.page_size / nand.ecc_size;
	nand.ecc_total 		= nand.ecc_steps * nand.ecc_bytes;

	nand.page_shift		= ffs(nand.page_size) - 1;

	switch (nand.page_size)
	{
		case 2048:
			nand.oob_ofs	= 64 - nand.ecc_total;
			break;
		case 4096:
			nand.oob_ofs	= 224 - nand.ecc_total;
			break;
		case 8192:
			nand.oob_ofs	= 448 - nand.ecc_total;
			break;
		default:
			NOTICE("pagesize %d is not support\n",nand.page_size);
			break;
	}

	NOTICE("Nand info: page %dB, %d address cycle, %d page shift\n",
	nand.page_size,
	nand.cycle,
	nand.page_shift);

	NOTICE("ECC info: %d bits ecc per %d Bytes, offset %d \n",
	nand.ecc_strength,
	nand.ecc_size,
	nand.oob_ofs);

	nand_reset();

	return 0;
}

static int nand_read_buf(u8 *buf, size_t len)
{
	int i;
	u32 *p = (u32 *)buf;

	for(i = 0; i < (len >> 2); i++)
		p[i] = nand_readl(NAND_IO);

	return len;
}

/*
 * routine to handle page access
 * the range(addr, addr+size) is within one page
 */
static int nand_do_read_ops(u8 *buf, u32 addr, size_t size)
{
	int eccsize = nand.ecc_size;
	int eccbytes = nand.ecc_bytes;
	int stat = 0;
	unsigned int page, col, start_unit, end_unit, start_unit_addr, unit_index;
	int readlen;
	int ecc_range_start, ecc_range_end;

	u32 ecc_out[MAX_ECCPOS_ENTRIES_LARGE/4];

	u8 *p_ecc_out = (u8 *)ecc_out;
	u32 *p = (u32 *)buf;

	page = get_cur_page(addr);
	/*get column address within a pge */
	col = addr & (nand.page_size - 1);

	INFO("Reading page 0x%x\n",page);

	/* STEP-1: Read OOB area */
	enable_nce(1);
	nand_writeb(NAND_CLE, NAND_CMD_READ0);

	nand_writeb(NAND_ALE, (nand.oob_ofs + nand.page_size) & 0xFF);
	nand_writeb(NAND_ALE, ((nand.oob_ofs + nand.page_size) >> 8) & 0xFF);
	nand_writeb(NAND_ALE, page & 0xFF);
	nand_writeb(NAND_ALE, (page >> 8) & 0xFF);
	if(5 == nand.cycle)
		nand_writeb(NAND_ALE, (page >> 16) & 0xFF);

	if(otp_nand_rb_sel())
		nand_writel(NAND_SW_RB_DETECT, 0);

	nand_writeb(NAND_CLE, NAND_CMD_READSTART);

	nand_ready();

	/* ECC is disabled when read OOB data */
	memset(p_ecc_out, 0, sizeof(ecc_out));
	nand_read_buf(p_ecc_out, nand.ecc_total);

	enable_nce(0);
	udelay(10);

	/* STEP-2: Read payload data */
	start_unit = col >> (ffs(eccsize) - 1);
	end_unit = (col + size - 1) >> (ffs(eccsize) - 1);
	start_unit_addr = start_unit * eccsize;
	col = col & (eccsize - 1);
	INFO("start unit=0x%x end unit=0x%x offset=0x%x\n",start_unit, end_unit, col);

	enable_nce(1);
	nand_writeb(NAND_CLE, NAND_CMD_READ0);

	nand_writeb(NAND_ALE, start_unit_addr & 0xFF);
	nand_writeb(NAND_ALE, (start_unit_addr >> 8) & 0xFF);
	nand_writeb(NAND_ALE, page & 0xFF);
	nand_writeb(NAND_ALE, (page >> 8) & 0xFF);
	if(5 == nand.cycle)
		nand_writeb(NAND_ALE, (page >> 16) & 0xFF);

	if(otp_nand_rb_sel())
		nand_writel(NAND_SW_RB_DETECT, 0);

	nand_writeb(NAND_CLE, NAND_CMD_READSTART);

	nand_ready();

	/* STEP-3: ecc correction */
	for(unit_index = start_unit; unit_index <= end_unit; unit_index++)
	{
		bch_decode_start(p_ecc_out + eccbytes * unit_index);
		for(readlen = 0; readlen < eccsize; readlen += 4)
		{
			if ( ((unit_index == start_unit) && (readlen < col))	\
				|| ((unit_index == end_unit) && (readlen >= col + size)) )
				nand_readl(NAND_IO);	/* dummy read*/
			else
				*(p++) = nand_readl(NAND_IO);
		}

		ecc_range_start	= (unit_index == start_unit)? col : 0;
		ecc_range_end	= (unit_index == end_unit)? (((col+size-1)&(eccsize - 1)) + 1) : eccsize;
		stat = bch_decode_correct(buf, ecc_range_start, ecc_range_end);
		if(stat)
			break;
		buf += readlen;
	}

	enable_nce(0);

	return stat;
}

/*
 * nand_read() can handle unaligned page access
 * addr Must be 4-bytes aligned
 * length Must be multiple of 4
 */
size_t nand_read(u32 *buffer, u32 addr, size_t length)
{
	int bytes, col;
	int ret = 0;
	size_t left = length;
	u8 *data = (u8 *)buffer;

	col = (addr & (nand.page_size - 1));

	do {
		bytes = min(nand.page_size - col, left);

		ret = nand_do_read_ops(data, addr, bytes);
		if(ret < 0)
			break;

		/* For subsequent reads align to page boundary */
		col = 0;

		left	-= bytes;
		data	+= bytes;
		addr	+= bytes;
	}while(left);

	return (length - left);
}

int sd_nand_init(void)
{
	return nand_init();
}

void sd_nand_deinit(void)
{
}

size_t sd_nand_read(int lba, uintptr_t buf, size_t size)
{
	uintptr_t ofs;
	ofs = lba * FLASH_BLOCK_SIZE;
	return nand_read((u32*)buf, ofs, size);
}
