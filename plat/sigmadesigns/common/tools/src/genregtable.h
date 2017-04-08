
#ifndef _GEN_REG_TABLE_H_
#define _GEN_REG_TABLE_H_

#define CONFIG_LITTLE_ENDIAN

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

#if 0
struct sEntryString
{
	unsigned short length;
	char data16[2];
	char data32[4];
};
#endif

struct sEntryUSB
{
	unsigned int config;
	char str[0];	/* the length of str is dynamic, it depends on USB group length */
};

struct sEntry8Bytes
{
	char data32_high[4];
	char data32_low[4];
};

#undef CFI_SUPPORT
#ifdef CFI_SUPPORT
//for cfi
struct sEntryCFIid
{
	unsigned short mid; //manufacture id
	unsigned short did; //device id1
	unsigned short did2; //device id2
	unsigned short did3; //device id3
};

struct CFIinfo
{
	unsigned short reg_addr;
	unsigned char data_err;
	unsigned char data_ok;
};
struct sEntryCFIinfo
{
	unsigned int length;
	struct CFIinfo cfiInfo;
};

struct sEntryCFIinfo2
{
	struct CFIinfo cfiInfo[2];
};
#endif

struct sEntryGeneric
{
	union {
		struct sEntryFirst entryFirst;
		struct sEntryConfig entryConfig;
		struct sEntryGroup entryGroup;
		struct sEntryRegister entryRegister;
		struct sEntryDDRRegister entryDDRRegister;
		struct sEntryDelay entryDelay;
#if 0
		struct sEntryString entryString;
#endif
		struct sEntryUSB entryUSB;
		struct sEntry8Bytes entry8Bytes;
#ifdef CFI_SUPPORT
		struct sEntryCFIid entryCFIid;
		struct sEntryCFIinfo entryCFIinfo;
		struct sEntryCFIinfo2 entryCFIinfo2;
#endif
	} u;
};

#define ENTRY_MAX_NUM  512

#define TABLE_SIZE (sizeof(struct sEntryGeneric) * ENTRY_MAX_NUM)

#define REG_MASK 0x1fffffff


#endif /* _GEN_REG_TABLE_H_ */
