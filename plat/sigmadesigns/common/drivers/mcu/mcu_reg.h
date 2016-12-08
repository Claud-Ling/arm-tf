#ifndef __MCU_REG_H__
#define __MCU_REG_H__

#include <mcu/mips00.h>
#include <mcu/mips20.h>

#ifndef __ASSEMBLY__

struct stb_regfile {
	volatile union MIPS00Reg mips00;/*+0x00*/	
	volatile uint8_t sram_addr[2];	/*+0x01,BE*/
	volatile uint8_t sram_data;	/*+0x03*/
	volatile uint8_t hole4[2];
	volatile uint8_t clksrc[2];	/*+0x06*/
	volatile uint8_t gpio4;		/*+0x08*/
	volatile uint8_t gpio5;		/*+0x09*/
	volatile uint8_t gpio10[2];	/*+0x0a*/
	volatile uint8_t hole11[2];
	volatile uint8_t read_cycle;	/*+0x0e*/
	volatile uint8_t hole15;
	volatile uint8_t hcmd[16];	/*+0x10*/
	volatile union MIPS20Reg mips20;/*+0x20,RO*/
	volatile uint8_t hole21[15];
	volatile uint8_t mcmd[16];	/*+0x30,RO*/
	volatile uint8_t spi_boot_addr[4];/*+0x40,BE*/
	volatile uint8_t hole44[44];	/*+0x44*/
};

#define DEFINE_STB_REGS(base)	\
struct stb_regfile *stb_regs = (struct stb_regfile*)(base)
#define DECLARE_STB_REGS	\
extern struct stb_regfile *stb_regs
#define STB_REG(nm) stb_regs-> nm

#endif /*!__ASSEMBLY__*/

#endif /*__MCU_REG_H__*/
