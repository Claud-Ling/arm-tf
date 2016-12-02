
#ifndef __REG_TABLE_H__
#define __REG_TABLE_H__

#define GROUP_TUNING	1
#define GROUP_BALANCE	2
#define GROUP_UMAC		3
#define GROUP_FLASH		4
#define GROUP_USB		5
#define GROUP_TUNING1		6

#ifndef __ASSEMBLY__
struct sEntryFirst
{
	char magic_code[2];
	unsigned short length;
	unsigned int crc32;
	unsigned int pad;
};

struct sEntryConfig
{
	unsigned int chip_id;
	unsigned short cpu_freq;
	unsigned short mem_freq;
	unsigned int pad;
};

struct sEntryGroup
{
	unsigned int group_id;
	unsigned short offset;
	unsigned short size;
	unsigned int pad;
};

struct sEntryRegister
{
	/* reg_address,
	 * bit31 reserve, bit30~bit29 access width, bit28~bit0 register address
	 */
	unsigned int reg_address; 
	unsigned int reg_data;
	unsigned int pad;
};

struct sEntryDDRRegister
{
	/* reg_address: bit28~bit0,register address.
	* reg_address[bit31] = 1, word acess.
	*	bit30~bit29 specify read/write/check operation. read=00'b, write=01'b, check=02'b, rdwr=03'b
	* reg_address[bit31] = 0, byte or halfword acess.
	*      reg_data[16] specify byte or halfword access.
	*      reg_data[16] =0, byte access.
	* 		reg_address[bit30~bit29] specify read/write/check/rdwr operation. read=00'b, write=01'b, check=02'b, rdwr=03'b
	*      reg_data[16] =1, halfword access.
	* 		reg_address[bit30~bit29] specify read/write/check/rdwr operation. read=00'b, write=01'b, check=02'b, rdwr=03'b	
	*
	* data_mask: masked with reg_data, to get effictive bits of reg_data.
	*/
	unsigned int reg_address; 
	unsigned int reg_data;
	unsigned int data_mask;
};

struct sEntryDelay
{
	unsigned int padding; /* padding data, no use */
	unsigned int usec; /* micro second */
	unsigned int pad;
};

struct sEntryUSB
{
	unsigned int config;
	char str[1];	/* the length of str is dynamic, it depends on USB group length */
};

struct sEntryGeneric
{
	union {
		struct sEntryFirst entryFirst;
		struct sEntryConfig entryConfig;
		struct sEntryGroup entryGroup;
		struct sEntryRegister entryRegister;
		struct sEntryDDRRegister entryDDRRegister;
		struct sEntryDelay entryDelay;
		struct sEntryUSB entryUSB;
	} u;
};

#define ENTRY_MAX_NUM 512

#define TABLE_SIZE (sizeof(struct sEntryGeneric) * ENTRY_MAX_NUM)

#define REG_MASK 0x1fffffff

extern int RegTableSetup(void);
extern int RegTableWriteGroup(unsigned int groupId);
#endif /*__ASSEMBLY__*/

#endif /* __REG_TABLE_H__ */
