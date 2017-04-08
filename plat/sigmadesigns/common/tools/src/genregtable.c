#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h> /* getopt */
//#include <netinet/in.h>


#include "crc32.h"
#include "genregtable.h"

#define DEBUG
#ifdef DEBUG
#define TRACE(x...) do {if(verbose) fprintf(stdout, ## x);} while(0)
#define TRACE_ERR(x...) do {if(verbose) fprintf(stderr, ## x);} while(0)
#else
#define TRACE(x...) 
#define TRACE_ERR(x...) 
#endif
//undef DUMP_REG_TABLE

//#define DUMP_REG_TABLE


int gen_reg_table(char *in_file[], const char *out_file);
int create_reg_table(char *in_file[], struct sEntryGeneric *entryGeneric);
int locate_reg_table(const char *out_file, unsigned long *pos);
int write_reg_table(const char *out_file, struct sEntryGeneric *entryGeneric, unsigned long pos);
static void invalid_input(const char *filename, int line, char *str);
void remove_space(char *str);
void remove_newline(char *str);
void remove_comment(char *str);
int str_empty(const char *p);
void host_to_target(struct sEntryGeneric *entry);
int target_to_host(struct sEntryGeneric *entry);
void endian_swap(unsigned char *buff, unsigned int len);
int dump_reg_table_into_script(const char *out_file, unsigned long pos);

static unsigned int htonl(unsigned int l);
static unsigned int ntohl(unsigned int l);
static unsigned short htons(unsigned short s);
static unsigned short ntohs(unsigned short s);

#define MAIN_VERSION 1
#define SUB_VERSION 0


#define BUFF_SIZE 1024*10

#define BOOT1_SIZE 0x10000

#define print_usage() \
	printf("Usage: %s [option] <in-file 1> ... <in-file n> <out-file>.\n" \
			"Options:\n" \
			"\t-p, position\n" \
			"\t-f, force to create\n" \
			"\t-c <cpu-frequency>, CPU frequency\n" \
			"\t-m <mem-frequency>, MEM frequency\n" \
			"\t-t, test output file\n" \
			"\t-g <group-config-file>, group config file\n" \
			"\t-d, dump register table into script file\n" \
			"\t-v, verbose\n" \
			"\t-V, version\n", argv[0])

//#define GROUOP_CFI 6

char *strGroups[16] = {"RESERVED-NOUSE", "TUNING", "BALANCE", "UMAC", "FLASH", NULL};

static int verbose = 0;
/*
 * register table default position in boot loader image.
 * default position is 0xfc00, 63KB.
 */
static long def_position = 0xfc00;
/*
 * user set position in command line.
 * if position is set, it has a higher priority than def_position.
 */
static long position = 0; 
/*
 * default CPU frequency, default value is 350MHz.
 * this is HiDTV PRO-UX|WX|QX standard setting
 */
static unsigned short def_cpu_freq = 350;
/*
 * user set CPU frequency in command line.
 * if cpu_freq is set, it will has a higher priority than def_cpu_freq
 * and CPU_FREQ command in script.
 */
static unsigned short cpu_freq = 0;
/*
 * default MEM frequency, default value is 400MHz.
 * this is HiDTV PRO-UX|WX|QX standard setting
 */
static unsigned short def_mem_freq = 400;
/*
 * user set MEM frequency in command line.
 * if mem_freq is set, it will has a higher priority than def_mem_freq
 * and MEM_FREQ command in script.
 */
static unsigned short mem_freq = 0;
/*
 * do we need to swap before write data?
 */
static int swap = 0;
/*
 * if there is no register table in output file,
 * genregtable will deny to write it in default.
 * if user set force by option -f, genregtable will write it anyway.
 */
static int force = 0;
/*
 * check if there is register table in output image file.
 */
static int output_check = 0;
/*
 * group config file path
 */
static char config_path[512];
/*
 * dump register table in the image file into a human readable script
 */
static int dump = 0;

static unsigned short regtab_len = 0;

extern int getgroups(const char *);

int main(int argc, char *argv[])
{
	int i;
	char *out_file = NULL;
	char *in_file[16] = {
		NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL, NULL};
	int ret = 0;

	int c;
	int argn;

	while((c = getopt(argc, argv, "p:c:m:tg:dvVf")) != EOF)
	{
		switch(c)
		{
			case 'p':
				position = strtol(optarg, NULL, 0);
				break;
			case 'c':
				cpu_freq = strtol(optarg, NULL, 0);
				break;
			case 'm':
				mem_freq = strtol(optarg, NULL, 0);
				break;
			case 't':
				output_check = 1;
				break;
			case 'g':
				strcpy(config_path, optarg);
				break;
			case 'd':
				dump = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'V':
				printf("Version: %d.%d\n", MAIN_VERSION, SUB_VERSION);
				return 0;
			case 'f':
				force = 1;
				break;
			default:
				print_usage();
				return 1;
		}
	}
	
	argn = argc - optind;
	if(argn < 1)
	{
		print_usage();
		return 1;
	}
	//printf("optind = %d,argn = %d\n",optind,argn);
	out_file = argv[optind + argn - 1];
	//printf("out_file = %s\n",out_file);
	for(i = 0; i < argn - 1; i++)
	{
		in_file[i] = argv[optind + i];
		//printf("in_file[%d] = %s\n",i,in_file[i]);
	}

	if(getgroups(config_path))
	{
		ret = gen_reg_table(in_file, out_file);
	}
	else
	{
		ret = 1;
	}

	return ret;
}

int gen_reg_table(char *in_file[], const char *out_file)
{
	//struct sEntryGeneric entryGeneric[ENTRY_MAX_NUM];
	//struct sEntryGeneric *entryGeneric;
	unsigned long pos = ~0ul;
	int ret;
	struct sEntryGeneric *entryGeneric;

	entryGeneric = (struct sEntryGeneric * )malloc(ENTRY_MAX_NUM*12);	


      // entryGeneric =(struct sEntryGeneric *)calloc(ENTRY_MAX_NUM,sizeof(struct sEntryGeneric));
	if(0 != (ret = locate_reg_table(out_file, &pos)))
	{
   	return ret;
	}

	if(dump)
	{
		dump_reg_table_into_script(out_file, pos);
	}
	else
	{
#if 0
		memset(&entryGeneric, 0, sizeof(entryGeneric));
		if(0 != (ret = create_reg_table(in_file, &entryGeneric[0])))
		{
			return ret;
		}
	
		if(0 != (ret = write_reg_table(out_file, &entryGeneric[0], pos)))
		{
			return ret;
		}
#else
		memset(entryGeneric, 0, ENTRY_MAX_NUM*sizeof(struct sEntryGeneric));
		if(0 != (ret = create_reg_table(in_file,entryGeneric)))
		{
			return ret;
		}
	
		if(0 != (ret = write_reg_table(out_file, entryGeneric, pos)))
		{
			return ret;

		}
#endif
	}

	return 0;
}

#ifdef DUMP_REG_TABLE
static void dump_reg_table(struct sEntryGeneric *entryGeneric)
{
	unsigned char *p = (unsigned char *)entryGeneric;
	int i;

	for(i = 0; i < 384; i += 12)
	//for(i = 0; i < TABLE_SIZE; i += 12)
	{
	printf("%d 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x\n", i/12 + 1, 
			p[i+0], p[i+1], p[i+2], p[i+3], 
			p[i+4], p[i+5], p[i+6], p[i+7],
			p[i+8], p[i+9], p[i+10], p[i+11]);
	}
}
#endif

int create_reg_table(char *in_file[], struct sEntryGeneric *entryGeneric)
{
	FILE *in_fp;
	int f_i;
	int entryNum = 2;
	struct sEntryFirst *entryFirst = &entryGeneric[0].u.entryFirst;
	struct sEntryConfig *entryConfig = &entryGeneric[1].u.entryConfig;
	struct sEntryGeneric *entryIndex = &entryGeneric[2];
	struct sEntryGroup *entryGroup = &entryGeneric[2].u.entryGroup;
	unsigned short table_len;
	int line;
	int curr_group = 0;

	char orig_buff[BUFF_SIZE]; 
	char buff[BUFF_SIZE]; 

	char err_msg[256];

	/* do some initialize firstly */ 
	memset((void *)entryGeneric, 0x0, TABLE_SIZE);

	entryFirst->magic_code[0] = 'R';
	entryFirst->magic_code[1] = 'U';
	entryFirst->length = 0;
	entryFirst->crc32 = 0;

	entryConfig->chip_id = 0;

	for(f_i = 0; in_file[f_i]; f_i++)
	{
		int buff_pos = 0;

		in_fp = fopen(in_file[f_i], "r");
		if(!in_fp)
		{
			invalid_input(in_file[f_i], 0, "Can't open file");
		}
		line = 0;
		while(fgets(buff+buff_pos, BUFF_SIZE, in_fp) && entryNum <= ENTRY_MAX_NUM)
		{
			unsigned int chip_id;
			unsigned int reg_addr;
			unsigned int reg_data;
			unsigned int data_mask;
			unsigned int usec;
			unsigned int tail_code;
			int str_len = 0;

			line++;
			remove_newline(buff+buff_pos);
			strcpy(orig_buff, buff);

			remove_comment(buff+buff_pos);
			remove_space(buff+buff_pos);
			if(str_empty(buff+buff_pos))
			{
				continue; /* we continue here to avoid entryNum++ */
			}

			/*get the really line sperated by '\'*/
			str_len = strlen(buff+buff_pos);
			buff_pos += str_len - 1;
			if (0x5c == buff[buff_pos]) { 
				continue;
			}
			else
			{
				buff_pos = 0;
			}
			remove_space(buff);
			/* add a tailing string, and try to get input from it
			 * this can force sscanf to check the content after the
			 * real last parameter
			 */
			strcat(buff, "TAILING 0x76543210");

			if(strncmp(buff, "CHIP_ID", 7) == 0)
			{
				if(2 == sscanf(buff, "CHIP_ID ( %x ) TAILING %x", &chip_id, &tail_code))
				{
					TRACE("Got chip ID : 0x%08x\n", chip_id);
					entryConfig->chip_id = chip_id;
					continue; /* we continue here to avoid entryNum++ */
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "CPU_FREQ", 8) == 0)
			{
				unsigned int freq;
				if(cpu_freq)
				{
					/* we have user set CPU frequency, just skip CPU_FREQ in script */
					continue;
				}
				if(2 == sscanf(buff, "CPU_FREQ ( %d ) TAILING %x", &freq, &tail_code))
				{
					TRACE("Got CPU frequency : 0x%04x\n", freq);
					cpu_freq = (unsigned short)freq;
					continue; /* we continue here to avoid entryNum++ */

				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "MEM_FREQ", 8) == 0)
			{
				unsigned int freq;
				if(mem_freq)
				{
					/* we have user set MEM frequency, just skip MEM_FREQ in script */
					continue;
				}
				if(2 == sscanf(buff, "MEM_FREQ ( %d ) TAILING %x", &freq, &tail_code))
				{
					TRACE("Got MEM frequency : 0x%04x\n", freq);
					def_mem_freq = (unsigned short)freq;
					continue; /* we continue here to avoid entryNum++ */

				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "[", 1) == 0)	/* Group Entry */
			{
				int j;
				for(j = 1; strGroups[j]; j++)
				{
					if(strncmp(buff + 1, strGroups[j], strlen(strGroups[j])) == 0 &&
							buff[strlen(strGroups[j]) + 1] == ']')
					{
						struct sEntryGeneric *entryTmp;
						TRACE("Group [%s] starts\n", strGroups[j]);
						curr_group = j;

						/*
						 * check whether there is a same type group exists.
						 */
						for(entryTmp = &entryGeneric[2]; (unsigned long)entryTmp < (unsigned long)entryGroup; entryTmp++)
						{
							if(entryTmp->u.entryGroup.group_id == curr_group)
							{
								sprintf(err_msg, "Group [%s] exists already.", strGroups[j]);
								invalid_input(in_file[f_i], line, err_msg);
							}
						}

						/*
						 * move all of the register and delay entry 1 position forward
						 * to make room for the group entry
						 */
						for(entryTmp = entryIndex; (unsigned long)entryTmp > (unsigned long)entryGroup; entryTmp--)
						{
							*entryTmp = *(entryTmp - 1);
						}

						/*
						 * set previous group size
						 */
						if((unsigned long)entryGroup > (unsigned long)&entryGeneric[2])
						{
							(entryGroup - 1)->size = ((unsigned long)entryIndex - 
													  (unsigned long)&entryGeneric[0] -
													  (entryGroup - 1)->offset);
						}

						/*
						 * adjust all of existing group entry
						 */
						for(entryTmp = &entryGeneric[2]; (unsigned long)entryTmp < (unsigned long)entryGroup; entryTmp++)
						{
							entryTmp->u.entryGroup.offset = 
								entryTmp->u.entryGroup.offset +
								sizeof(struct sEntryGeneric);
						}

						/*
						 * write current group entry
						 */
						entryGroup->group_id = j;
						entryGroup->offset = (unsigned long)(entryIndex + 1) - 
							(unsigned long)&entryGeneric[0];
						entryGroup->size = 0;
                        
						entryIndex++;
						entryGroup++;
						break;
					}
				}
				if(NULL == strGroups[j])
				{
					sprintf(err_msg, "Unknown group: %s", orig_buff);
					invalid_input(in_file[f_i], line, err_msg);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_SET_REG_B", 18) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}

				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_SET_REG_B ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%02x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x3 << 29));
					entryIndex->u.entryRegister.reg_data = reg_data;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_SET_REG_HW", 19) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_SET_REG_HW ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%04x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x2 << 29));
					entryIndex->u.entryRegister.reg_data = reg_data;
					entryIndex++;				
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_SET_REG_W", 18) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_SET_REG_W ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x0 << 29));
					entryIndex->u.entryRegister.reg_data = reg_data;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_CLRBIT_REG_B", 21) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_CLRBIT_REG_B ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x3 << 29) | (1 << 31));
					entryIndex->u.entryRegister.reg_data = ((reg_data & 0xFF) | (1 << 16));
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_CLRBIT_REG_HW", 22) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_CLRBIT_REG_HW ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x2 << 29) | (1 << 31));
					entryIndex->u.entryRegister.reg_data = ((reg_data & 0xFFFF) | (1 << 16));
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_CLRBIT_REG_W", 21) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_CLRBIT_REG_W ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x0 << 29) | (1 << 31) | (1 << 0));
					entryIndex->u.entryRegister.reg_data = reg_data;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_SETBIT_REG_B", 21) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_SETBIT_REG_B ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x3 << 29) | (1 << 31));
					entryIndex->u.entryRegister.reg_data = ((reg_data & 0xFF) | (0 << 16));
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_SETBIT_REG_HW", 22) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_SETBIT_REG_HW ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x2 << 29) | (1 << 31));
					entryIndex->u.entryRegister.reg_data = ((reg_data & 0xFFFF) | (0 << 16));
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "ASSEMBLY_SETBIT_REG_W", 21) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				
				/*since tuning.S not support the old interface*/
				if(!strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "can't be used for DDR tuning");
				}
				
				if(3 == sscanf(buff, "ASSEMBLY_SETBIT_REG_W ( %x , %x ) TAILING %x", &reg_addr, &reg_data, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x0 << 29) | (1 << 31) | (0 << 0));
					entryIndex->u.entryRegister.reg_data = reg_data;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
/* add for DDR initial training for Pro_SXL */
			else if(strncmp(buff, "DDR_REG_WRITE_B", 15) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif	
				if(4 == sscanf(buff, "DDR_REG_WRITE_B ( %x , %x , %x ) TAILING %x", &reg_addr, &reg_data, &data_mask, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%02x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x1<< 29));
					entryIndex->u.entryDDRRegister.reg_data = ((reg_data & 0xFF));
					entryIndex->u.entryDDRRegister.data_mask = data_mask;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_WRITE_HW", 16) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(4 == sscanf(buff, "DDR_REG_WRITE_HW ( %x , %x , %x ) TAILING %x", &reg_addr, &reg_data,  &data_mask, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%04x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x1<< 29));
					entryIndex->u.entryDDRRegister.reg_data = ((reg_data & 0xFFFF) | (1 << 16));
					entryIndex->u.entryDDRRegister.data_mask = data_mask;
					entryIndex++;				
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_WRITE_W", 15) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(4 == sscanf(buff, "DDR_REG_WRITE_W ( %x , %x , %x ) TAILING %x", &reg_addr, &reg_data, &data_mask, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK) | (0x1<< 31) | (1 << 29));
					entryIndex->u.entryDDRRegister.reg_data = reg_data;
					entryIndex->u.entryDDRRegister.data_mask = data_mask;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_CHECK_B", 15) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0		
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(4 == sscanf(buff, "DDR_REG_CHECK_B ( %x , %x , %x ) TAILING %x", &reg_addr, &reg_data, &data_mask, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK) |(2 << 29));
					entryIndex->u.entryDDRRegister.reg_data = ((reg_data & 0xFF));
					entryIndex->u.entryDDRRegister.data_mask = data_mask;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_CHECK_HW", 16) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(4 == sscanf(buff, "DDR_REG_CHECK_HW ( %x , %x , %x ) TAILING %x", &reg_addr, &reg_data, &data_mask, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = ((reg_addr & REG_MASK) | (0x2 << 29));
					entryIndex->u.entryDDRRegister.reg_data = ((reg_data & 0xFFFF) | (1 << 16));
					entryIndex->u.entryDDRRegister.data_mask = data_mask;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_CHECK_W", 15) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(4 == sscanf(buff, "DDR_REG_CHECK_W ( %x , %x , %x ) TAILING %x", &reg_addr, &reg_data, &data_mask, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK)  | (1 << 31)  | (0x2 << 29));
					entryIndex->u.entryDDRRegister.reg_data = reg_data;
					entryIndex->u.entryDDRRegister.data_mask = data_mask;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_READ_B", 14) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(2 == sscanf(buff, "DDR_REG_READ_B ( %x ) TAILING %x", &reg_addr, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = ((reg_addr & REG_MASK) );
					entryIndex->u.entryDDRRegister.reg_data =0;
					entryIndex->u.entryDDRRegister.data_mask =0;
					printf("DDR_REG_READ_B %x \n",entryIndex->u.entryDDRRegister.reg_address);
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_READ_HW", 15) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(2 == sscanf(buff, "DDR_REG_READ_HW ( %x ) TAILING %x", &reg_addr, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = ((reg_addr & REG_MASK));
					entryIndex->u.entryDDRRegister.reg_data =  (1 << 16);
					printf("DDR_REG_READ_HW %x \n",entryIndex->u.entryDDRRegister.reg_address);
                                        entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_READ_W", 14) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(2 == sscanf(buff, "DDR_REG_READ_W ( %x ) TAILING %x", &reg_addr, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK) | (1 << 31) | (0 << 29));
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_RDWR_B", 14) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(2 == sscanf(buff, "DDR_REG_RDWR_B(%x,%x)", &reg_addr, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = ((reg_addr & REG_MASK) | 0x3 << 29);
					//entryIndex->u.entryDDRRegister.reg_data = 0;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_RDWR_HW", 15) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(2 == sscanf(buff, "DDR_REG_RDWR_HW ( %x ) TAILING %x", &reg_addr, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = ((reg_addr & REG_MASK)| 0x3 << 29);
					entryIndex->u.entryDDRRegister.reg_data =  (1 << 16);
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "DDR_REG_RDWR_W", 14) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
			#if 0	
				/*since lib/regtable.c not support the old interface*/
				if(strncmp(strGroups[curr_group], "TUNING", 6))
				{
					invalid_input(in_file[f_i], line, "only can be used for DDR tuning");
				}
			#endif
				if(2 == sscanf(buff, "DDR_REG_RDWR_W ( %x ) TAILING %x", &reg_addr, &tail_code))
				{
					TRACE("Got register address : 0x%08x, data : 0x%08x\n",
							reg_addr, reg_data);
					entryIndex->u.entryDDRRegister.reg_address = 
						((reg_addr & REG_MASK) | (1 << 31) | (0 << 29) | 0x3 << 29);
					//entryIndex->u.entryDDRRegister.reg_data = reg_data;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
/* add for DDR initial training for Pro_SXL */
			else if(strncmp(buff, "U_DELAY", 7) == 0)
			{
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}

				if(2 == sscanf(buff, "U_DELAY ( %d ) TAILING %x", &usec, &tail_code))
				{
					TRACE("Got micro delay : %d\n", usec);
					entryIndex->u.entryDelay.padding = 0;
					entryIndex->u.entryDelay.usec = usec;
					entryIndex++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "USB_ACTIVE_LEVEL", 16) == 0)
			{
				unsigned int usb_active_level;
				if(2 == sscanf(buff, "USB_ACTIVE_LEVEL ( %d ) TAILING %x", &usb_active_level, &tail_code))
				{
					struct sEntryUSB *entryUSB;

					if(0 != strcmp("USB", strGroups[curr_group]))
					{
						invalid_input(in_file[f_i], line, orig_buff);
					}
					TRACE("Got usb active level : 0x%04x\n", usb_active_level);
					if(usb_active_level)
						usb_active_level = 1;

                entryUSB = (struct sEntryUSB *)(entryGeneric +(entryGroup - 1)->offset / sizeof(struct sEntryGeneric));
                                        entryUSB->config |= usb_active_level;

					continue; /* we continue here to avoid entryNum++ */					
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			}
			else if(strncmp(buff, "UPDATE_FILE_NAME", 16) == 0)
			{
				char *p = buff + 16, *q;
				char ufnStr[256];
				struct sEntryUSB *entryUSB;
				char *usbStr;
				int nPadding, nEntries;

				if(0 != strcmp("USB", strGroups[curr_group]))
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			 entryUSB = (struct sEntryUSB *)(entryGeneric + (entryGroup - 1)->offset /12);
                                usbStr = entryUSB->str;
				remove_space(p);
				if('(' != *p)
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
				q = strchr(p, ')');
				if(!q)
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
				p++;
				*q = 0;
				remove_space(p);
				if(0 != strlen(usbStr))
					sprintf(ufnStr, ":UFN=%s", p);
				else
					sprintf(ufnStr, "UFN=%s", p);
				nPadding = (sizeof(entryUSB->config) + strlen(usbStr)) % sizeof(struct sEntryGeneric);
				nEntries = ((strlen(ufnStr) - nPadding) + sizeof(struct sEntryGeneric) - 1) / sizeof(struct sEntryGeneric);
				if(0 == strlen(usbStr))
					nEntries += 1;

strcat((char *)entryUSB->str, ufnStr);
				
				entryIndex += nEntries;
				entryNum += nEntries;
				continue; /* we continue here to avoid entryNum++ */
				
#if 0
				int str_len;
				char *p = buff + 16;
				char *q,*q0;
				while('(' != *p)
				{
					if('\0' == *p)
					{
						invalid_input(in_file[f_i], line, orig_buff);
					}
					p++;
				}
				p++;
				remove_space(p);
				q = p;
				while(')' != *q && ' ' != *q )//file name should include blank character
				{
					if('\0' == q++)
					{
						invalid_input(in_file[f_i], line, orig_buff);
					}
				}
				*q = '\0';
				q0 = q;
				str_len = (q - p);
				entryIndex->u.entryString.length = (0xffff & str_len) | file_name_length;
				if(str_len <= 6) { //6byte left, since string length take 2 byts
					str_len = 6;
				} else {
					str_len -= 6;
					str_len = (str_len + 0x7) & (~0x7); //8 bytes aligned
					str_len += 6;
				}				
				q = p + str_len;
				while(q0 < q)
				{
					*q = '\0';
					q--;
				}				
				memcpy(entryIndex->u.entryString.data16, p, str_len);
				entryIndex += ((str_len >> 3) + 1);
				entryNum += ((str_len >> 3) + 1);
				continue; /* we continue here to avoid entryNum++ */					
#endif
			
                       }
			else if(strncmp(buff, "FORCE_NO_UPDATE", 15) == 0)
			{
				char *p = buff + 15, *q;
				char fnuStr[256];
				struct sEntryUSB *entryUSB;
				char *usbStr;
				int nPadding, nEntries;

				if(0 != strcmp("USB", strGroups[curr_group]))
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
			 entryUSB = (struct sEntryUSB *)(entryGeneric + (entryGroup - 1)->offset /12);
				
                                usbStr = entryUSB->str;

				remove_space(p);
				if('(' != *p)
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
				q = strchr(p, ')');
				if(!q)
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}
				p++;
				*q = 0;
				remove_space(p);
				if(0 != strlen(usbStr))
					sprintf(fnuStr, ":FNU=%s", p);
				else
					sprintf(fnuStr, "FNU=%s", p);

				nPadding = (sizeof(entryUSB->config) + strlen(usbStr)) % 
					sizeof(struct sEntryGeneric);
				nEntries = ((strlen(fnuStr) - nPadding) + sizeof(struct sEntryGeneric) - 1) / 
					sizeof(struct sEntryGeneric);
				if(0 == strlen(usbStr))
					nEntries += 1;
				strcat(usbStr, fnuStr);
				entryIndex += nEntries;
				entryNum += nEntries;
				continue; /* we continue here to avoid entryNum++ */
		
                      }
#if CFI_SUPPORT
			else if(strncmp(buff, "CFI_VALUE_FIXUP", 15) == 0)
			{
				unsigned int mid,did,did2,did3,addr,data_err,data_ok;
				int i,ok = 1,firstCFIinfo = 1,hasOneCFI = 0;
				char tmp[40];
				char *p = NULL,*q = NULL;
				struct sEntryCFIinfo * pCFIinfo;
				struct sEntryCFIinfo2 entryCFIinfo2;
				if(0 == curr_group)
				{
					invalid_input(in_file[f_i], line, "does NOT belong to any group");
				}
				if(5 == sscanf(buff, "CFI_VALUE_FIXUP ( %x , %x , %x , %x , %x ,", &mid, &did, &did2, &did3,&addr))
				{
					TRACE("Got CFI ID : %04x,%04x,%04x,%04x\n",mid,did,did2,did3);
					entryIndex->u.entryCFIid.mid = mid & 0xffff;
					entryIndex->u.entryCFIid.did = did & 0xffff;
					entryIndex->u.entryCFIid.did2 = did2 & 0xffff;
					entryIndex->u.entryCFIid.did3 = did3 & 0xffff;
					entryIndex++;
					entryNum++;
				}
				else
				{
					invalid_input(in_file[f_i], line, orig_buff);
				}

				p = buff;
				for(i=0;i<4;i++)
				{
					if(NULL == (p=strchr(p,',')))
					{
						invalid_input(in_file[f_i], line, orig_buff);
					}
					else
					{
						p++;
					}
				}

				q = p;
				while(NULL != q)
				{
					q = p;
					for(i=0;i<3;i++)
					{
						if(NULL == (q=strchr(q,',')))
						{
							ok = 0;
							break;
						}
						else
						{
							ok = 1;
							q++;
						}
					}
					if(1 == ok)
					{
						memcpy(tmp,p,q-p);
						tmp[q-p] = '\0';
						strcat(tmp, "TAILING 0x76543210");
						if(4 == sscanf(tmp, "%x , %x , %x , TAILING %x", &addr, &data_err, &data_ok, &tail_code))
						{
							TRACE("Got CFI revise information : 0x%04x, data_err : 0x%02x, data_ok : 0x%02x\n",addr, data_err, data_ok);
							if(firstCFIinfo)
							{
								entryIndex->u.entryCFIinfo.length = 0;
								entryIndex->u.entryCFIinfo.cfiInfo.reg_addr = addr & 0xffff;
								entryIndex->u.entryCFIinfo.cfiInfo.data_err = data_err & 0xff;
								entryIndex->u.entryCFIinfo.cfiInfo.data_ok = data_ok & 0xff;
								pCFIinfo = (struct sEntryCFIinfo *)entryIndex;
								firstCFIinfo = 0;
								entryIndex++;
								entryNum++;
							}
							else
							{
								if(!hasOneCFI)
								{
									entryCFIinfo2.cfiInfo[0].reg_addr = addr & 0xffff;
									entryCFIinfo2.cfiInfo[0].data_err = data_err & 0xff;
									entryCFIinfo2.cfiInfo[0].data_ok = data_ok & 0xff;
									hasOneCFI = 1;
								}
								else
								{
									entryCFIinfo2.cfiInfo[1].reg_addr = addr & 0xffff;
									entryCFIinfo2.cfiInfo[1].data_err = data_err & 0xff;
									entryCFIinfo2.cfiInfo[1].data_ok = data_ok & 0xff;
									memcpy((char *)entryIndex,&entryCFIinfo2,sizeof(struct sEntryCFIinfo2));
									hasOneCFI = 0;
									entryIndex++;
									entryNum++;
								}
							}
								
						}
						else
						{
							invalid_input(in_file[f_i], line, orig_buff);
						}
					}
					else
					{
						if(2 == i) //last one
						{
							if(4 == sscanf(p, "%x , %x , %x ) TAILING %x", &addr, &data_err, &data_ok, &tail_code))
							{
								TRACE("Got CFI revise information : 0x%04x, data_err : 0x%02x, data_ok : 0x%02x\n",addr, data_err, data_ok);
								if(!hasOneCFI)
								{
									entryCFIinfo2.cfiInfo[0].reg_addr = addr & 0xffff;
									entryCFIinfo2.cfiInfo[0].data_err = data_err & 0xff;
									entryCFIinfo2.cfiInfo[0].data_ok = data_ok & 0xff;
									entryCFIinfo2.cfiInfo[1].reg_addr = 0;
									entryCFIinfo2.cfiInfo[1].data_err = 0;
									entryCFIinfo2.cfiInfo[1].data_ok = 0;
									memcpy((char *)entryIndex,&entryCFIinfo2,sizeof(struct sEntryCFIinfo2));
									entryIndex++;

								}
								else
								{
									entryCFIinfo2.cfiInfo[1].reg_addr = addr & 0xffff;
									entryCFIinfo2.cfiInfo[1].data_err = data_err & 0xff;
									entryCFIinfo2.cfiInfo[1].data_ok = data_ok & 0xff;
									memcpy((char *)entryIndex,&entryCFIinfo2,sizeof(struct sEntryCFIinfo2));
									entryIndex++;
								}
								if(NULL == pCFIinfo)
								{
									invalid_input(in_file[f_i], line, "does NOT has any revise data");
								}
								else
								{
									pCFIinfo->length = (unsigned int)(entryIndex) - (unsigned int)pCFIinfo; //not include flash id
								}
							}
							else
							{
								invalid_input(in_file[f_i], line, orig_buff);
							}	
						}
						else
						{
							invalid_input(in_file[f_i], line, orig_buff);
						}
					}
					p = q;
				}
				
			}
#endif
			else
			{
				invalid_input(in_file[f_i], line, orig_buff);
			}

			entryNum++;
		}
		fclose(in_fp);
	}

#ifdef DUMP_REG_TABLE
	printf("==== entryGroup--.\n");
	dump_reg_table(entryGeneric);
#endif
	entryGroup--;
	if((unsigned long)entryGroup >= (unsigned long)&entryGeneric[2])
	{
		entryGroup->size = (unsigned long)entryIndex - 
				(unsigned long)&entryGeneric[0] -
				entryGroup->offset;
	}

	if(cpu_freq == 0)
	{
		fprintf(stderr, "CPU_FREQ must be set\n");
		exit(1);
	}
	
	table_len = entryNum * sizeof(struct sEntryGeneric);
	entryFirst->length = table_len;

	/* set CPU frequency */
	entryConfig->cpu_freq = (cpu_freq ? cpu_freq : def_cpu_freq);
	/* set MEM frequency */
	entryConfig->mem_freq = (mem_freq ? mem_freq : def_mem_freq);

	entryFirst->crc32 = crc32(0, &entryGeneric[0], 4);
	entryFirst->crc32 = crc32(entryFirst->crc32, &entryGeneric[1], 
			table_len - sizeof(struct sEntryGeneric));

#ifdef DUMP_REG_TABLE
	printf("==== create Register table to be written into target.\n");
	dump_reg_table(entryGeneric);
#endif

	return 0;
}

static int detect_magic_code(const unsigned char *buff, int *swapped)
{
	unsigned char local_buff[sizeof(struct sEntryGeneric) * ENTRY_MAX_NUM];
	struct sEntryFirst *entry = (struct sEntryFirst *)&local_buff[0];
		
	unsigned char local_buff2[sizeof(struct sEntryGeneric) * ENTRY_MAX_NUM];
	struct sEntryFirst *entry2 = (struct sEntryFirst *)&local_buff2[0];
	

	memcpy(local_buff, buff, sizeof(struct sEntryFirst));
	memcpy(local_buff2, buff, sizeof(struct sEntryFirst));

	if(entry->magic_code[0] == 'R' &&
		entry->magic_code[1] == 'U')
	{
		/* do further check */
		memcpy(local_buff, buff, TABLE_SIZE);
		if(0 == target_to_host((struct sEntryGeneric *)local_buff))
		{
			*swapped = 0;
			return 1;
		}
	}

	endian_swap(local_buff2, sizeof(struct sEntryFirst));
	if(entry2->magic_code[0] == 'R' &&
		entry2->magic_code[1] == 'U')
	{
		/* do further check */
		memcpy(local_buff2, buff, TABLE_SIZE);
		endian_swap(local_buff2, TABLE_SIZE);
		if(0 == target_to_host((struct sEntryGeneric *)local_buff2))
		{
			*swapped = 1;
			return 1;
		}
	}

	return 0;
}

int locate_reg_table(const char *out_file, unsigned long *pos)
{
	FILE *out_fp;
	unsigned int file_len = 0;
	unsigned char *out_buff = NULL;
	unsigned long t_pos[2] = {position, def_position};
	int swapped;
	int i;
	int ret = 0;

	*pos = ~0ul;

	out_fp = fopen(out_file, "r+b");
	if(!out_fp)
	{
		//printf("Failed to open: %s in %s[%d] %p\n",out_file,__func__,__LINE__,out_fp);
		/* file does not exist */
		*pos = 0ul;		/* position 0 */
		swap = 0;		/* we don't need swap */
		goto out;
	}

	fseek(out_fp, 0, SEEK_END);
	file_len = ftell(out_fp);
	fseek(out_fp, 0, SEEK_SET);

	/* add extra TABLE_SIZE to avoid access violation in detect_magic_code */
	out_buff = (unsigned char *)calloc(file_len + TABLE_SIZE, sizeof(unsigned char));
	if(!out_buff)
	{
		fprintf(stderr, "Out of memory!\n");
		exit (1);
	}

	fread(out_buff, file_len, 1, out_fp);
	fclose(out_fp);

	/* detect position and def_position firstly */
	for(i = 0; i < 2; i++)
	{
		if(t_pos[i] < file_len)
		{
			if(detect_magic_code(out_buff + t_pos[i], &swapped))
			{
				*pos = t_pos[i];
				swap = swapped;
				/**
				 * check wether the size of boot1 is larger than 64k
				 */
				if ((*pos + regtab_len) > BOOT1_SIZE) {
					fprintf(stderr,"the size of boot1 is larger than %x.\n",BOOT1_SIZE);
					ret = 1;
				}
				goto out;
			}
		}
	}

	/* do full scan */
	for(i = 0; i < file_len; i += 8)
	{
		if(detect_magic_code(out_buff + i, &swapped))
		{
			*pos = i;
			swap = swapped;
			/**
			 * check wether the size of boot1 is larger than 64k
			 */
			if ((*pos + regtab_len) > BOOT1_SIZE) {
				fprintf(stderr,"the size of boot1 is larger than %x.\n",BOOT1_SIZE);
				ret = 1;
			}
			goto out;
		}
	}
	/*
	 * no register table in output file, 
	 * return error without force option.
	 */
	if(!force)
	{
		fprintf(stderr, "Target image file %s contains no register table.\n", out_file);
		ret = 1;
	}

out:
	if(out_buff)
	{
		free(out_buff);
	}

	if(output_check)
	{
		exit(ret);
	}
	return ret;
}

static int do_dump_generic_group_script(char *buff, struct sEntryGroup *entryGroup)
{
	int i;
   	if (strcasecmp(strGroups[entryGroup->group_id],"TUNING")==0)
		return 0;
	for(i = 0; i < entryGroup->size; i += sizeof(struct sEntryGeneric))
	{
		unsigned int firstWord;
		unsigned short offset = entryGroup->offset;
		firstWord = *(unsigned int *)(buff + offset + i);
		if(0 == firstWord)
		{
			struct sEntryDelay *entryDelay = (struct sEntryDelay *)(buff + offset + i);
			fprintf(stdout, "\tU_DELAY(%d)\n", entryDelay->usec);
		}
		else
		{
			struct sEntryRegister *entryRegister = (struct sEntryRegister *)(buff + offset + i);
			int width = ((entryRegister->reg_address & ~REG_MASK) >> 29) & 0x3;
			unsigned int reg_address = entryRegister->reg_address & REG_MASK;
			unsigned int reg_data = entryRegister->reg_data;
			if(!(entryRegister->reg_address >> 31)) { //set operation
				switch(width)
				{
					case 0:
						fprintf(stdout, "\tASSEMBLY_SET_REG_W(0x%x, 0x%08x)\n", reg_address, reg_data);
						break;
					case 2:
						fprintf(stdout, "\tASSEMBLY_SET_REG_HW(0x%x, 0x%04x)\n", reg_address, reg_data);
						break;
					case 3:
						fprintf(stdout, "\tASSEMBLY_SET_REG_B(0x%x, 0x%02x)\n", reg_address, reg_data);
						break;
					default:
						return -1;
						break;
				}
			}else //or & clear operation
			{
				switch(width)
				{
					case 0://word
						switch(reg_address & 0x3) {
						case 0:// or  operation
							fprintf(stdout, "\tASSEMBLY_SETBIT_REG_W(0x%x, 0x%08x)\n", reg_address & 0xfffffffc, reg_data);
							break;
						case 1://clear operation
							fprintf(stdout, "\tASSEMBLY_CLRBIT_REG_W(0x%x, 0x%08x)\n", reg_address & 0xfffffffc, reg_data);
							break;
						default:
							return -1;
						}
						break;
					case 2://half word
						switch((reg_data >> 16) &0xff) {
						case 0:
							fprintf(stdout, "\tASSEMBLY_SETBIT_REG_HW(0x%x, 0x%04x)\n", reg_address, reg_data & 0xffff);
							break;
						case 1:
							fprintf(stdout, "\tASSEMBLY_CLRBIT_REG_HW(0x%x, 0x%04x)\n", reg_address, reg_data & 0xffff);
							break;
						default:
							return -1;
						}
						break;
					case 3://byte
						switch((reg_data >> 16) &0xff) {
						case 0:
							fprintf(stdout, "\tASSEMBLY_SETBIT_REG_B(0x%x, 0x%02x)\n", reg_address, reg_data & 0xff);
							break;
						case 1:
							fprintf(stdout, "\tASSEMBLY_CLRBIT_REG_B(0x%x, 0x%02x)\n", reg_address, reg_data & 0xff);
							break;
						default:
							return -1;
						}
						break;
					default:
						return -1;
						break;
				}
			}
		}
	}

	return 0;
}
void debugtuning(char *buff,struct sEntryGroup *entryGroup)
{
	int i;
	unsigned int offset = entryGroup->offset;

	for(i = 0; i < entryGroup->size; i += 12)
    {
	struct sEntryDDRRegister *entryRegister = (struct sEntryDDRRegister *)(buff + offset + i);
	printf ("0x%08x , 0x%08x ,0x%08x \n",entryRegister->reg_address,entryRegister->reg_data,entryRegister->data_mask);
		
	}
}
static int do_dump_generic_group_script_fortuning(char *buff, struct sEntryGroup *entryGroup)
{
	int i;
   // debugtuning(buff,entryGroup);
	/*targeting groups: TUNING,BALANCE,and TUNING1*/
	//if (strcasecmp(strGroups[entryGroup->group_id],"TUNING")==0)
	{
  //printf("entryGroup->size is %d",entryGroup->size);
	for(i = 0; i < entryGroup->size; i += sizeof(struct sEntryDDRRegister))
	{
		unsigned int firstWord;
		unsigned short offset = entryGroup->offset;
		firstWord = *(unsigned int *)(buff + offset + i);
		if(0 == firstWord)
		{
			struct sEntryDelay *entryDelay = (struct sEntryDelay *)(buff + offset + i);
			fprintf(stdout, "\tU_DELAY(%d)\n", entryDelay->usec);
		}
		else
		{
			struct sEntryDDRRegister *entryRegister = (struct sEntryDDRRegister *)(buff + offset + i);
			int wordornot = ((entryRegister->reg_address & ~REG_MASK) >> 29) ;
			unsigned int reg_address = entryRegister->reg_address & REG_MASK;
			unsigned int reg_data = entryRegister->reg_data;
			unsigned int data_mask = entryRegister->data_mask;
		//	printf(" address 0x%08x data 0x%08x mask 0x%08x \n",reg_address,reg_data,data_mask);
			if (wordornot&0x4) //for word operation
				{
				int woperation= (wordornot&0x3);
            switch (woperation){
             case 0:
							   fprintf(stdout, "\tDDR_REG_READ_W(0x%x)\n", reg_address);
							   break;
						case 1:
							   fprintf(stdout, "\tDDR_REG_WRITE_W(0x%08x, 0x%x, 0x%x)\n", reg_address, reg_data,data_mask);
							   break;
						case 2:
                               fprintf(stdout, "\tDDR_REG_CHECK_W(0x%08x, 0x%x, 0x%x)\n", reg_address, reg_data,data_mask);
							   break;
						case 3:
							   fprintf(stdout, "\tDDR_REG_RDWR_W(0x%08x)\n", reg_address);
							   break;
						default:
							return -1;
				        }
			} else { //for half word or byte
            int boperation=(reg_data>>16)&0x01;
             
			switch (boperation){
       case  0:   //byte access
            switch (wordornot&0x3) {
				            case 0:
											fprintf(stdout, "\tDDR_REG_READ_B(0x%08x)\n", reg_address);
											break;
										case 1:
											fprintf(stdout, "\tDDR_REG_WRITE_B(0x%08x, 0x%x, 0x%x)\n", reg_address, reg_data & 0xff,data_mask);
											break;
										case 2:
											fprintf(stdout, "\tDDR_REG_CHECK_B(0x%08x, 0x%0x, 0x%x)\n", reg_address, reg_data & 0xff,data_mask);
											break;	
										case 3:
											fprintf(stdout, "\tDDR_REG_RDWR_B(0x%08x)\n", reg_address);
											break;		
										default:
											return -1;
					                       }
					  	
			   	    break;
			   case  1:   //half word access
			   			 switch (wordornot&0x3) {
				            case 0:
											fprintf(stdout, "\tDDR_REG_READ_HW(0x%08x)\n", reg_address);
											break;
										case 1:
											fprintf(stdout, "\tDDR_REG_WRITE_HW(0x%08x, 0x%x, 0x%x)\n", reg_address, reg_data & 0xffff,data_mask);
											break;
										case 2:
											fprintf(stdout, "\tDDR_REG_CHECK_HW(0x%08x, 0x%x, 0x%x)\n", reg_address, reg_data & 0xffff,data_mask);
											break;	
										case 3:
											fprintf(stdout, "\tDDR_REG_RDWR_HW(0x%08x)\n", reg_address);
											break;		
										default:
											return -1;
					                 }
					  	
			   	      break;
			   default:
						return -1;		  
			}
			
			
		  }
	     }
		}
	}
	return 0;
}

static int do_dump_usb_group_script(char *buff, struct sEntryGroup *entryGroup)
{
	unsigned short offset = entryGroup->offset;
	unsigned short size = entryGroup->size;
	struct sEntryUSB *entryUSB = (struct sEntryUSB *)(buff + offset);
	char *str = entryUSB->str;
	char usbFileName[256];
	int lenLeft = size - sizeof(entryUSB->config);

	fprintf(stdout, "\tUSB_ACTIVE_LEVEL(%d)\n", (entryUSB->config & 1) ? (1) : (0) );
	while(((unsigned long)str) < ((unsigned long)(buff + offset + size)))
	{
		char *p;
		int len;

		p = strchr(str, ':');

		if(0 == strncmp("UFN=", str, strlen("UFN=")))	/* UPDATE_FILE_NAME */
		{
			str += strlen("UFN=");
			lenLeft -= strlen("UFN=");
			if(p)
				len = p - str;
			else
				len = strlen(str);
			if(len > lenLeft)
				len = lenLeft;
			strncpy(usbFileName, str, len);
			usbFileName[len] = 0;
			fprintf(stdout, "\tUPDATE_FILE_NAME(%s)\n", usbFileName);
			if(p)
				len += 1; /* skip separator ':' */
			str += len;	/* move to next string */
			lenLeft -= len;
		}
		else if(0 == strncmp("FNU=", str, strlen("FNU=")))	/* FORCE_NO_UPDATE */
		{
			str += strlen("FNU=");
			lenLeft -= strlen("FNU=");
			if(p)
				len = p - str;
			else
				len = strlen(str);
			if(len > lenLeft)
				len = lenLeft;
			strncpy(usbFileName, str, len);
			usbFileName[len] = 0;
			fprintf(stdout, "\tFORCE_NO_UPDATE(%s)\n", usbFileName);
			if(p)
				len += 1; /* skip separator ':' */
			str += len;	/* move to next string */
			lenLeft -= len;
		}
		else
		{
			/* we reach padding area, just break */
			break;
		}
	}

	return 0;
}

static int do_dump_reg_table_into_script(char *buff)
{
	//struct sEntryFirst *entryFirst = (struct sEntryFirst *)buff;
	struct sEntryConfig *entryConfig = (struct sEntryConfig *)(buff + sizeof(struct sEntryGeneric));
	struct sEntryGroup *entryGroup = (struct sEntryGroup *)(buff + sizeof(struct sEntryGeneric) * 2);
	int i=2;
	//unsigned int length = entryFirst->length;
	unsigned int chip_id = entryConfig->chip_id;
	unsigned short cpu_freq = entryConfig->cpu_freq;
	unsigned short mem_freq = entryConfig->mem_freq;
	
	if(chip_id)
	{
		fprintf(stdout, "CHIP_ID(%x)\n", chip_id);
	}
	if(cpu_freq)
	{
		fprintf(stdout, "CPU_FREQ(%d)\n", cpu_freq);
	}
	if(mem_freq)
	{
		fprintf(stdout, "MEM_FREQ(%d)\n", mem_freq);
	}
	
	while(0< entryGroup->group_id && entryGroup->group_id < 32)
	{
		fprintf(stdout, "[%s]\n", strGroups[entryGroup->group_id]);
		if(NULL == strGroups[entryGroup->group_id])
		{
			fprintf(stderr, "Unknown group id: %d\n", entryGroup->group_id);
			return 1;
		}
	
          if (strcmp(strGroups[entryGroup->group_id],"TUNING")==0 ||
	      strcmp(strGroups[entryGroup->group_id],"TUNING1")==0 ||
	      strcmp(strGroups[entryGroup->group_id],"BALANCE")==0 )
			  {
			do_dump_generic_group_script_fortuning(buff,entryGroup);
		
       } else {
		   if (!strcasecmp(strGroups[entryGroup->group_id], "USB")) {
			    if(do_dump_usb_group_script(buff, entryGroup) < 0)
			    {   printf("break point 1\n");
				     return 1;
			     }
		        } else {
		   
			     if(do_dump_generic_group_script(buff, entryGroup) < 0)
			      {
			       printf("break point 2\n");
				     return 1;
			       }
		  }
		}
	 //   entryGroup++;
	  i++;
		entryGroup=(struct sEntryGroup *)(buff + sizeof(struct sEntryGeneric) * i);
	}
	
	return 0;
}

int dump_reg_table_into_script(const char *out_file, unsigned long pos)
{
	unsigned char buff[TABLE_SIZE];
	FILE *out_fp;
	int ret;
	
	out_fp = fopen(out_file, "rb");
	if(!out_fp)
	{
		return 1;
	}
	
	fseek(out_fp, pos, SEEK_SET);
	fread(buff, 1, TABLE_SIZE, out_fp);
	
	if(swap)
	{
		endian_swap(buff, TABLE_SIZE);
	}
	
	if(0 != target_to_host((struct sEntryGeneric *)buff))
	{
		fclose(out_fp);
		return 1;
	}
	
	ret = do_dump_reg_table_into_script((char*)buff);
	
	fclose(out_fp);
	return ret;
}

int write_reg_table(const char *out_file, struct sEntryGeneric *entryGeneric, unsigned long pos)
{
	FILE *out_fp;
	unsigned int file_len = 0;
	unsigned int total_len = 0;
	unsigned int length = ((struct sEntryFirst *)entryGeneric)->length;
	/*
	 * actual position, 
	 * if position is set, use position,
	 * if pos is set (find register table in image file), use pos,
	 * otherwise, use def_position.
	 */
	unsigned long act_pos = def_position;
	unsigned char *out_buff = NULL;

#ifdef DUMP_REG_TABLE
	printf("came into write_reg_table.\n");
	dump_reg_table(entryGeneric);
#endif
	out_fp = fopen(out_file, "r+b");
	if(!out_fp)
	{
		file_len = 0;
	}else{
		fseek(out_fp, 0, SEEK_END);
		file_len = ftell(out_fp);
		fseek(out_fp, 0, SEEK_SET);
	}

	if(position)
	{
		if(pos != ~0ul && pos != position)
		{
			fprintf(stdout, "Found existing register table pos: 0x%08lx differs to user assigned pos: 0x%08lx\n", pos, position);
		}
		act_pos = position;
	}
	else if(pos != ~0ul)
	{
		act_pos = pos;
	}

	total_len = act_pos + length;
	if(file_len > total_len)
		total_len = file_len;

	out_buff = (unsigned char *)calloc(total_len, sizeof(unsigned char));
	if(!out_buff)
	{
		fprintf(stderr, "Out of memory!\n");
		exit (1);
	}

	if(out_fp)
	{
		fread(out_buff, file_len, 1, out_fp);
		fclose(out_fp);
	}
	out_fp = fopen(out_file, "wb");
	if(!out_fp)
	{
		fprintf(stdout, "Can't open output file %s.\n", out_file);
		free(out_buff);
		return 3;
	}

	fprintf(stdout, "  Register table CRC32: 0x%08x\n",
			entryGeneric[0].u.entryFirst.crc32);

#ifdef DUMP_REG_TABLE
	printf("before host_to_target.\n");
	dump_reg_table(entryGeneric);
#endif
	/* last step, swap */
	host_to_target(&entryGeneric[0]);
	
#ifdef DUMP_REG_TABLE
	printf("before endian_swap.\n");
	dump_reg_table(entryGeneric);
#endif
        if(swap)
	{
		endian_swap((unsigned char *)entryGeneric, length);
	}

#ifdef DUMP_REG_TABLE
	printf("end of endian_swap.\n");
	dump_reg_table(entryGeneric);
#endif
	memcpy(out_buff + act_pos, &entryGeneric[0], length);

	fseek(out_fp, 0, SEEK_SET);
	fwrite(out_buff, total_len, 1, out_fp);

	free(out_buff);

	fclose(out_fp);
	return 0;
}

static void invalid_input(const char *filename, int line, char *str)
{
	fprintf(stderr, "%s[%d]: invalid: \"%s\"\n", filename, line, str);
	fprintf(stderr, "Maybe you should update your tool\n");

	exit(1);
}

int str_empty(const char *p)
{
	if (!p)
		return (1);
	for (; *p; p++)
		if (!isspace(*p))
			return (0);
	return (1);
}

void remove_newline(char *str)
{
	char *p;

	p = strchr(str, '\n');
	if(p)
	{
		*p = 0;
	}
	p = strchr(str, '\r');
	if(p)
	{
		*p = 0;
	}
}

void remove_comment(char *str)
{
	char *p;
	
	p = strchr(str, '#');
	if(p)
	{
		*p = 0;
	}
}

/*
 * remove leading and tailing space tab characters
 */
void remove_space(char *str)
{
	char t[BUFF_SIZE] = {'\0',};
	char *p, *q;
	int tail_pos;

	memset(t, 0, sizeof(t));

	p = str;
	while(*p != 0)
	{
		if(isspace(*p))
		{
			p++;
			continue;
		}
		else
		{
			break;
		}
	}

	tail_pos = strlen(p) - 1;
	q = p + tail_pos;
	while(1)
	{
		if(isspace(*q))
		{
			*q = '\0';
			q--;
		}
		else
		{
			break;
		}
	}

	strncpy(t, p, BUFF_SIZE - 1);
	t[BUFF_SIZE - 1] = 0;

	strcpy(str, t);
}

void one_group_host_to_target(struct sEntryGeneric *entry,int length,int type)
{
	int i;
	unsigned int firstWord;
	struct sEntryRegister *entryReg;
	struct sEntryDDRRegister *entryReg1;
	struct sEntryDelay *entryDelay;

	for(i=0;i<length;i++)
	{
		firstWord = *(unsigned int *)(entry + i);
		if(0 == firstWord)
		{
			/* a delay entry */
			entryDelay = (struct sEntryDelay *)(entry +i);
			entryDelay->usec = htonl(entryDelay->usec);
		}
		else
		{
			/* a register entry */
		if(type)
                        {
			entryReg1 = (struct sEntryDDRRegister *)(entry + i);
			entryReg1->reg_address = htonl(entryReg1->reg_address);
			entryReg1->reg_data = htonl(entryReg1->reg_data);
                        entryReg1->data_mask = htonl(entryReg1->data_mask);  
                           } else{
                	entryReg = (struct sEntryRegister *)(entry + i);
			entryReg->reg_address = htonl(entryReg->reg_address);
			entryReg->reg_data = htonl(entryReg->reg_data);
		          }
                }
	}
	
}

#ifdef CFI_SUPPORT
void cfi_group_host_to_target(struct sEntryGeneric *entry,int length)
{
	int i,j = 0;
	unsigned int one_cfi_len;
	struct sEntryCFIid * entryCFIid = (struct sEntryCFIid *)entry;
	struct sEntryCFIinfo * entryCFIinfo = (struct sEntryCFIid *)(entry + 1);
	struct sEntryCFIinfo2 * entryCFIinfo2;

	while(j < length)
	{
		entryCFIid = (struct sEntryCFIid *)(entry + j);
		entryCFIinfo = (struct sEntryCFIid *)(entry + j + 1);
		entryCFIid->mid = htons(entryCFIid->mid);
		entryCFIid->did = htons(entryCFIid->did);
		entryCFIid->did2 = htons(entryCFIid->did2);
		entryCFIid->did3 = htons(entryCFIid->did3);
	
		one_cfi_len = entryCFIinfo->length;
		entryCFIinfo->length = htonl(entryCFIinfo->length);
		entryCFIinfo->cfiInfo.reg_addr = htons(entryCFIinfo->cfiInfo.reg_addr);
	
		for(i = 2; i < one_cfi_len / sizeof(struct sEntryGeneric) + 1; i ++)
		{
			entryCFIinfo2 = (struct sEntryCFIinfo2 *)(entry + j + i);
			entryCFIinfo2->cfiInfo[0].reg_addr = htons(entryCFIinfo2->cfiInfo[0].reg_addr);
			entryCFIinfo2->cfiInfo[1].reg_addr = htons(entryCFIinfo2->cfiInfo[1].reg_addr);
		}
	
		j += i;
	}
	
}
#endif

void usb_group_host_to_target(struct sEntryGeneric *entry,int length)
{
	struct sEntryUSB *entryUSB = (struct sEntryUSB *)entry;

	entryUSB->config = htonl(entryUSB->config);
#if 0
	int i, count;
	struct sEntry8Bytes * entry8Bytes = (struct sEntry8Bytes *)entry;
	struct sEntryString *entryString = (struct sEntryString *)entry;
	entryString->length = htons(entryString->length);
#endif
}

void host_to_target(struct sEntryGeneric *entry)
{
	struct sEntryFirst *entryFirst = (struct sEntryFirst *)entry;
	struct sEntryConfig *entryConfig = (struct sEntryConfig *)(entry + 1);

	struct sEntryGroup *entryGroup;

	unsigned short length = entryFirst->length;
	unsigned short i;

	entryFirst->length = htons(entryFirst->length);
	entryFirst->crc32 = htonl(entryFirst->crc32);

	entryConfig->chip_id = htonl(entryConfig->chip_id);
	entryConfig->cpu_freq = htons(entryConfig->cpu_freq);
	entryConfig->mem_freq = htons(entryConfig->mem_freq);

	for(i = 2; i < length / sizeof(struct sEntryGeneric); i ++)
	{
		unsigned int group_num;
		int group_offset,group_size;
		
		group_num = *(unsigned int *)(entry + i);
		if(group_num <= 0 || group_num >= 32)
		{
			break;
		}

		entryGroup = (struct sEntryGroup *)(entry + i);
		group_offset = entryGroup->offset;
		group_size = entryGroup->size;

		entryGroup->group_id = htonl(entryGroup->group_id);
		entryGroup->offset = htons(entryGroup->offset);
		entryGroup->size = htons(entryGroup->size);
		/*cfi is handled seperately*/
		if(group_num && (!strcmp(strGroups[group_num],"CFI")))
		{
#if CFI_SUPPORT
			cfi_group_host_to_target((struct sEntryGeneric *)(entry + group_offset/12),group_size/12);
#endif
		}
		else if(group_num && (!strcmp(strGroups[group_num],"USB")))
		{
			usb_group_host_to_target((struct sEntryGeneric *)(entry + group_offset/12),group_size/12);
		}
		else
		{
                        
		   if(group_num && (!strcmp(strGroups[group_num],"TUNING") ||
				    !strcmp(strGroups[group_num],"TUNING1") ||
				    !strcmp(strGroups[group_num],"BALANCE")))
		one_group_host_to_target((struct sEntryGeneric *)(entry + group_offset/12),group_size/12,1);
                else
                
		one_group_host_to_target((struct sEntryGeneric *)(entry + group_offset/12),group_size/12,0);
		
                 }		
	}
}

#if 0
void host_to_target(struct sEntryGeneric *entry)
{
	struct sEntryFirst *entryFirst = (struct sEntryFirst *)entry;
	struct sEntryConfig *entryConfig = (struct sEntryConfig *)(entry + 1);

	struct sEntryGroup *entryGroup;
	struct sEntryRegister *entryReg;
	struct sEntryDelay *entryDelay;

	unsigned short length = entryFirst->length;
	unsigned short i;

	entryFirst->length = htons(entryFirst->length);
	entryFirst->crc32 = htonl(entryFirst->crc32);

	entryConfig->chip_id = htonl(entryConfig->chip_id);
	entryConfig->cpu_freq = htons(entryConfig->cpu_freq);
	entryConfig->mem_freq = htons(entryConfig->mem_freq);

	for(i = 2; i < length / sizeof(struct sEntryGeneric); i ++)
	{
		unsigned int firstWord;

		firstWord = *(unsigned int *)(entry + i);

		if(0 < firstWord && firstWord < 32)
		{
			/* a group entry */
			entryGroup = (struct sEntryGroup *)(entry + i);
			entryGroup->group_id = htonl(entryGroup->group_id);
			entryGroup->offset = htons(entryGroup->offset);
			entryGroup->size = htons(entryGroup->size);
		}
		else if(0 == firstWord)
		{
			/* a delay entry */
			entryDelay = (struct sEntryDelay *)(entry +i);
			entryDelay->usec = htonl(entryDelay->usec);
		}
		else
		{
			/* a register entry */
			entryReg = (struct sEntryRegister *)(entry + i);
			entryReg->reg_address = htonl(entryReg->reg_address);
			entryReg->reg_data = htonl(entryReg->reg_data);
		}
	}
}
#endif

void one_group_target_to_host(struct sEntryGeneric *entry,int length,int type)
{
	int i;
	unsigned int firstWord;
	struct sEntryRegister *entryReg;
	struct sEntryDDRRegister *entryReg1;
	struct sEntryDelay *entryDelay;

	for(i=0;i<length;i++)
	{
		firstWord = *(unsigned int *)(entry + i);
		if(0 == firstWord)
		{
			/* a delay entry */
			entryDelay = (struct sEntryDelay *)(entry +i);
			entryDelay->usec = ntohl(entryDelay->usec);
		}
		else
		{
			/* a register entry */
		    if (type)
		    {
            entryReg1 = (struct sEntryDDRRegister *)(entry + i);
			entryReg1->reg_address = ntohl(entryReg1->reg_address);
			entryReg1->reg_data = ntohl(entryReg1->reg_data); 
			entryReg1->data_mask= ntohl(entryReg1->data_mask); 
			}else{
			entryReg = (struct sEntryRegister *)(entry + i);
			entryReg->reg_address = ntohl(entryReg->reg_address);
			entryReg->reg_data = ntohl(entryReg->reg_data); 
		      }
		}
	}
}

#ifdef CFI_SUPPORT
void cfi_group_target_to_host(struct sEntryGeneric *entry,int length)
{
	int i,j = 0;
	unsigned int one_cfi_len;
	struct sEntryCFIid * entryCFIid = (struct sEntryCFIid *)entry;
	struct sEntryCFIinfo * entryCFIinfo = (struct sEntryCFIid *)(entry + 1);
	struct sEntryCFIinfo2 * entryCFIinfo2;

	while(j < length)
	{
		entryCFIid = (struct sEntryCFIid *)(entry + j);
		entryCFIinfo = (struct sEntryCFIid *)(entry + j + 1);
		entryCFIid->mid = ntohs(entryCFIid->mid);
		entryCFIid->did = ntohs(entryCFIid->did);
		entryCFIid->did2 = ntohs(entryCFIid->did2);
		entryCFIid->did3 = ntohs(entryCFIid->did3);
	
		one_cfi_len = entryCFIinfo->length;
		entryCFIinfo->length = ntohl(entryCFIinfo->length);
		entryCFIinfo->cfiInfo.reg_addr = ntohs(entryCFIinfo->cfiInfo.reg_addr);
	
		for(i = 2; i < one_cfi_len / sizeof(struct sEntryGeneric) + 1; i ++)
		{
			entryCFIinfo2 = (struct sEntryCFIinfo2 *)(entry + j + i);
			entryCFIinfo2->cfiInfo[0].reg_addr = ntohs(entryCFIinfo2->cfiInfo[0].reg_addr);
			entryCFIinfo2->cfiInfo[1].reg_addr = ntohs(entryCFIinfo2->cfiInfo[1].reg_addr);
		}
	
		j += i;
	}
}
#endif

void usb_group_target_to_host(struct sEntryGeneric *entry, int length)
{
	struct sEntryUSB *entryUSB = (struct sEntryUSB *)entry;

	entryUSB->config = ntohl(entryUSB->config);
#if 0
	int i, count;
	struct sEntry8Bytes * entry8Bytes = (struct sEntry8Bytes *)entry;
	struct sEntryString *entryString = (struct sEntryString *)entry;
	entryString->length = ntohs(entryString->length);
	//printf("%d: %x : %s\n",__LINE__,entryString->length, entryString->data16);
#endif
}

int target_to_host(struct sEntryGeneric *entry)
{
	struct sEntryFirst *entryFirst = (struct sEntryFirst *)entry;
	struct sEntryConfig *entryConfig = (struct sEntryConfig *)(entry + 1);

	struct sEntryGroup *entryGroup;
#if 0
	struct sEntryRegister *entryReg;
	struct sEntryDelay *entryDelay;
#endif

	unsigned short length = ntohs(entryFirst->length);
	unsigned short i;

	unsigned int crc32_check;

	regtab_len = length;
	if(length > (ENTRY_MAX_NUM * sizeof(struct sEntryGeneric)) ||
		length % sizeof(struct sEntryGeneric) != 0)
	{
    printf("length is not right is %d \n",length);
		return 1;
	}
	entryFirst->length = ntohs(entryFirst->length);
	entryFirst->crc32 = ntohl(entryFirst->crc32);

	entryConfig->chip_id = ntohl(entryConfig->chip_id);
	entryConfig->cpu_freq = ntohs(entryConfig->cpu_freq);
	entryConfig->mem_freq = ntohs(entryConfig->mem_freq);
	for(i = 2; i < length / sizeof(struct sEntryGeneric); i ++)
	{
#if 0
		unsigned int firstWord;

		firstWord = ntohl(*(unsigned int *)(entry + i));
		if(0 < firstWord && firstWord < 32)
		{
			/* a group entry */
			entryGroup = (struct sEntryGroup *)(entry + i);
			entryGroup->group_id = ntohl(entryGroup->group_id);
			entryGroup->offset = ntohs(entryGroup->offset);
			entryGroup->size = ntohs(entryGroup->size);
		}
		else if(0 == firstWord)
		{
			/* a delay entry */
			entryDelay = (struct sEntryDelay *)(entry +i);
			entryDelay->usec = ntohl(entryDelay->usec);
		}
		else
		{
			/* a register entry */
			entryReg = (struct sEntryRegister *)(entry + i);
			entryReg->reg_address = ntohl(entryReg->reg_address);
			entryReg->reg_data = ntohl(entryReg->reg_data);
		}
#else
		unsigned int group_num;
		
		group_num =  ntohl(*(unsigned int *)(entry + i));
		if(group_num <= 0 || group_num >= 32)
		{
			break;
		}

		entryGroup = (struct sEntryGroup *)(entry + i);
		entryGroup->group_id = ntohl(entryGroup->group_id);
		entryGroup->offset = ntohs(entryGroup->offset);
		entryGroup->size = ntohs(entryGroup->size);
		/*cfi is handled seperately*/
		if(group_num && (!strcmp(strGroups[group_num],"CFI")))
		{
#ifdef CFI_SUPPORT
			cfi_group_target_to_host((struct sEntryGeneric *)(entry + entryGroup->offset/8),entryGroup->size/8);
#endif
		}
		if(group_num && (!strcmp(strGroups[group_num],"USB")))
		{	//printf("trace 333333333\n");
	//	usb_group_target_to_host((struct sEntryGeneric *)(entry + entryGroup->offset/8),entryGroup->size/8);
			usb_group_target_to_host((struct sEntryGeneric *)(entry + entryGroup->offset/12),entryGroup->size/12);
		}
		else
		{	//printf("trace 222222222\n");
		  if(group_num && (!strcmp(strGroups[group_num],"TUNING") ||
				   !strcmp(strGroups[group_num],"TUNING1") ||
				   !strcmp(strGroups[group_num],"BALANCE")))
		    one_group_target_to_host((struct sEntryGeneric *)(entry + entryGroup->offset/12),entryGroup->size/12,1);
		  else
		    one_group_target_to_host((struct sEntryGeneric *)(entry + entryGroup->offset/12),entryGroup->size/12,0);
		}
#endif
	}

#ifdef DUMP_REG_TABLE
	printf("Existing register table in target file.\n");
	dump_reg_table(entry);
#endif

	crc32_check = crc32(0, (void *)entry, 4);
	crc32_check = crc32(crc32_check, (void *)(entry + 1), length - sizeof(*entry));
	if(crc32_check != entryFirst->crc32)
		return 1;
	return 0;
}

void endian_swap(unsigned char *buff, unsigned int len)
{
	int i;

	if(len % 4)
	{
		TRACE("Warning: Endian swap length is not 4*n.\n");
	}

	for(i = 0; i < len; i += 4, buff += 4)
	{
		unsigned char c;
		c = *(buff + 0);
		*(buff + 0) = *(buff + 3);
		*(buff + 3) = c;
		c = *(buff + 2);
		*(buff + 2) = *(buff + 1);
		*(buff + 1) = c;
	}
}

#if defined(CONFIG_BIG_ENDIAN)
static unsigned int htonl(unsigned int l)
{
	return ((l >> 24) & 0xff) | (((l >> 16) & 0xff) << 8) | (((l >> 8) & 0xff) << 16) | ((l & 0xff) << 24);
}

static unsigned int ntohl(unsigned int l)
{
	return ((l >> 24) & 0xff) | (((l >> 16) & 0xff) << 8) | (((l >> 8) & 0xff) << 16) | ((l & 0xff) << 24);
}

static unsigned short htons(unsigned short s)
{
	return ((s >> 8) & 0xff) | ((s & 0xff) << 8);
}

static unsigned short ntohs(unsigned short s)
{
	return ((s >> 8) & 0xff) | ((s & 0xff) << 8);
}

#elif defined(CONFIG_LITTLE_ENDIAN)

static unsigned int htonl(unsigned int l)
{
	return l;
}

static unsigned int ntohl(unsigned int l)
{
	return l;
}

static unsigned short htons(unsigned short s)
{
	return s;
}

static unsigned short ntohs(unsigned short s)
{
	return s;
}

#else
#error "endian error for regtable"
#endif
