
#include <stdio.h>
#include <stdint.h>
#include <mmio.h>
#include <debug.h>
#include <assert.h>
#include <delay_timer.h>
#include <platform.h>
#include <sd_private.h>
#include <ddrsetting.h>
#include "regtable.h"
#include "ddr.h"

#define RT_SRAM_START	(CONFIG_SOCRAM_START + CONFIG_RT_OFS_RAM)
#define INVALID_GROUP_ID 0
#define NB_GROUPS	5
#define ROUND(v, a) ( (a) ? ((v) / (a) * (a)) : (v))
#ifndef NULL
# define NULL (void*)0
#endif

#define put32(u) tf_printf("%x", u)

#ifdef DEBUG_DDR
# define TRACE_SIMPLE_ENTRY(s,a)	do{			\
		printf(#s"(0x%x)\n", a);			\
  }while(0)
# define TRACE_ENTRY(s,a,d,m)	do{				\
	if (m)							\
		printf(#s"(%08x, %08x, %08x)\n", a, d, m);	\
	else							\
		printf(#s"(%08x, %08x)\n", a, d);		\
  }while(0)
# define TRACE_LOAD(o,l)	do{				\
	printf("load rtbl at ofs %#08x len %#x\n", o, l);	\
  }while(0)
#else
# define TRACE_SIMPLE_ENTRY(s,a) do{}while(0)
# define TRACE_ENTRY(s,a,d,m)	do{}while(0)
# define TRACE_LOAD(o,l)	do{}while(0)
#endif

/*Reload is not supported in ATF case*/
#define RegTableReload(ofs, len) do {		\
	panic();				\
}while(0)

#define ReadRegWord(a)  mmio_read_32(a)
#define ReadRegHWord(a) mmio_read_16(a)
#define ReadRegByte(a)  mmio_read_8(a)
#define WriteRegWord(a, v) do{ 		\
		mmio_write_32(a, v);	\
	}while(0)
#define WriteRegHWord(a, v) do{ 	\
		mmio_write_16(a, v);	\
	}while(0)
#define WriteRegByte(a, v) do{ 		\
		mmio_write_8(a, v);	\
	}while(0)

static const char rt_banner[] = "\nPLL:"CONFIG_RT_INFO_PLL"\nDDR:"CONFIG_RT_INFO_DDR
				"\nLVDS:"CONFIG_RT_INFO_PANEL"\nOTH:"CONFIG_RT_INFO_OTH"\n";

static struct regtblhdr{
	unsigned int valid:1;
	unsigned int pos:14;
	unsigned int len:14;
	struct {
		unsigned int id:4;
		unsigned int ofs:14;
		unsigned int sz:14;
	}grp[NB_GROUPS];
} rthdr = { .valid = 0,};

static int RegTableFindGroup(const unsigned int groupId, unsigned int *groupAddr, unsigned int *groupSize)
{
	int ret = 1;
	if (rthdr.valid)
	{
		int i = 0;
		for (i = 0; i < NB_GROUPS; i++)
		{
			if (rthdr.grp[i].id != INVALID_GROUP_ID && groupId == rthdr.grp[i].id)
			{
				*groupAddr = rthdr.grp[i].ofs;
				*groupSize = rthdr.grp[i].sz;
				ret = 0;
				break;
			}
		}
	}
	return ret;
}

int RegTableSetup(void)
{
	struct sEntryFirst *entryFirst = NULL;
	struct sEntryGroup *entryGroup = NULL;
	int i, hlen; /* default error */

	/*load tbl*/
	entryFirst = (struct sEntryFirst *)reg_tables;
	entryGroup = (struct sEntryGroup *)(reg_tables + sizeof(struct sEntryGeneric) * 2);

	/*reset*/
	rthdr.valid = 0;
	for(i = 0; i < NB_GROUPS; i++)
		rthdr.grp[i].id = INVALID_GROUP_ID;

	/*check magic_code*/
	if(entryFirst->magic_code[0] != 'R' || entryFirst->magic_code[1] != 'U')
	{
		ERROR("Not a valid reg table\n");
		return 1;
	}

	/*search groups*/
	if(entryGroup->group_id < 1 || entryGroup->group_id > 6)
	{
		ERROR("Invalid group id %d\n", entryGroup->group_id);
		return 2;
	}

	i = 0;
	hlen = entryGroup->offset;
	for(; ((uintptr_t)entryGroup - (uintptr_t)entryFirst) < hlen; entryGroup++)
	{
		if(entryGroup->group_id < 1 || entryGroup->group_id > 6)
		{
			ERROR("Invalid group id %d\n", entryGroup->group_id);
			continue;
		}
		rthdr.grp[i].id = entryGroup->group_id;
		rthdr.grp[i].ofs = entryGroup->offset;
		rthdr.grp[i].sz = entryGroup->size;
		i++;
	}

	rthdr.pos = 0;
	rthdr.len = ROUND(SD_RT_SIZE, sizeof(struct sEntryGeneric));
	rthdr.valid = 1;
	return 0;
}

int RegTableWriteGroup(unsigned int groupId)
{
	unsigned int offset = 0, size = 0;
	int ret = 0;
	int i, j;

	ret = RegTableFindGroup(groupId, &offset, &size);
	if(ret)
	{
		NOTICE("Cant find group: %d\n", groupId);
		return ret;
	}

	if (0 == size)
		goto OUT;	/*take care of zero length group*/

	if (offset < rthdr.pos || offset >= (rthdr.pos + rthdr.len)) {
		putchar('R');
		RegTableReload(offset, CONFIG_RT_RAM_SIZE);
		offset = 0;
	} else {
		offset -= rthdr.pos;
	}

	for(i = j = 0; j < size; i += sizeof(struct sEntryGeneric), j += sizeof(struct sEntryGeneric))
	{
		#define RT_ENTRY(ofs, idx) (reg_tables + (ofs) + (idx))
		struct sEntryGeneric *entry = NULL;
		if (!((offset + i) < rthdr.len))
		{
			putchar('R');
			RegTableReload(rthdr.pos + offset + i, CONFIG_RT_RAM_SIZE);
			offset = i = 0;
		}

		putchar('I');
		entry = (struct sEntryGeneric *)RT_ENTRY(offset, i);
		if(entry->u.entryDelay.padding == 0)
		{
			struct sEntryDelay *entryDelay = (struct sEntryDelay *)entry;
			TRACE_SIMPLE_ENTRY(U_DELAY, entryDelay->usec);
			udelay(entryDelay->usec);
		}
		else
		{
			struct sEntryDDRRegister *entryRegister = (struct sEntryDDRRegister *)entry;
			int ops = entryRegister->reg_address >> 29;
			unsigned int reg_address = (entryRegister->reg_address & REG_MASK) | 0xf0000000;
			unsigned int reg_data = entryRegister->reg_data;
			unsigned int reg_mask = entryRegister->data_mask;
			if (ops == 3) {
				// Do the hook from ddrsetting config file.
				// UMAC0:  DDR_REG_RDWR_B(0x00000000, 0x0)
				// UMAC1:  DDR_REG_RDWR_B(0x00000001, 0x0)
				TRACE_ENTRY(DDR_REG_RDWR_B, reg_address, reg_data, 0);
				ddr_do_dqs_gating(reg_address & 0xf);
			} else if (ops == 1 || ops == 5) {
				// DDR_REG_WRITE_B
				// DDR_REG_WRITE_HW
				// DDR_REG_WRITE_W
				int ops = 0;
				int width = ((entryRegister->reg_address & ~REG_MASK) >> 29) & 0x7;
				if (width == 1) {
					width = (entryRegister->reg_data >> 16) & 1;
					if (width == 1)
						ops = 2;
					else
						ops = 1;
				} else if (width == 5) {
					ops = 3;
				} else
					puts("Incorrect register entry operations!\n");

				switch(ops)
					{
						case 3:
							TRACE_ENTRY(DDR_REG_WRITE_W, reg_address, reg_data, reg_mask);
							WriteRegWord(reg_address, (ReadRegWord(reg_address) & (unsigned int)(~reg_mask)) | reg_data);
							break;
							
						case 2:
							TRACE_ENTRY(DDR_REG_WRITE_HW, reg_address, reg_data, reg_mask);
							WriteRegHWord(reg_address, (ReadRegHWord(reg_address) & (unsigned short)(~reg_mask)) | reg_data);
							break;
							
						case 1:
							TRACE_ENTRY(DDR_REG_WRITE_B, reg_address, reg_data, reg_mask);
							WriteRegByte(reg_address, (ReadRegByte(reg_address) & (unsigned char)(~reg_mask)) | reg_data);
							break;
							
						default:
							puts("Incorrect register entry width!\n");
							break;
					}
			} else if (ops == 2 || ops == 6) {
				// DDR_REG_CHECK_B
				// DDR_REG_CHECK_HW
				// DDR_REG_CHECK_W
				int ops = 0;
				int width = ((entryRegister->reg_address & ~REG_MASK) >> 29) & 0x7;
				if (width == 2) {
					width = (entryRegister->reg_data >> 16) & 1;
					if (width == 1)
						ops = 2;
					else
						ops = 1;
				} else if (width == 6) {
					ops = 3;
				} else
					puts("Incorrect register entry operations!\n");

				switch(ops)
				{
					case 3:
						TRACE_ENTRY(DDR_REG_CHECK_W, reg_address, reg_data, reg_mask);
						while (1)
							if ((ReadRegWord(reg_address) & reg_mask) == (reg_data & reg_mask))
								break;
						break;
					case 2:
						TRACE_ENTRY(DDR_REG_CHECK_HW, reg_address, reg_data, reg_mask);
						while (1)
							if ((ReadRegHWord(reg_address) & reg_mask) == (reg_data & reg_mask))
								break;
						break;
					case 1:
						TRACE_ENTRY(DDR_REG_CHECK_B, reg_address, reg_data, reg_mask);
						while (1)
							if ((ReadRegByte(reg_address) & reg_mask) == (reg_data & reg_mask))
								break;
						break;
					default:
						puts("Incorrect register entry width!\n");
						break;
				}
			} else
				puts("Incorrect register entry operations!\n");
		}
	}
OUT:
	putchar('A');
	putchar('D');
	putchar('A');
	putchar('D');

	if (GROUP_TUNING == groupId) puts(rt_banner);
	return 0;
}

