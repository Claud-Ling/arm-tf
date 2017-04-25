/*****************************************
  Copyright 2016
  Sigma Designs, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/
/**
  @file   pman_sec_tab.h
  @brief  describe pman_sec tab for downloading pman settings.
          evolved from bus_protection.c

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-05
  */

#include <stdint.h>

#ifndef __PMAN_SEC_TAB_H__
#define __PMAN_SEC_TAB_H__

#define VERSION_MAJOR		1
#define VERSION_MINOR		0
#define PST_VERSION(a,b)	((((a)&0xf)<<4) | ((b)&0xf))
#define PST_VERSION_CODE	PST_VERSION(VERSION_MAJOR, VERSION_MINOR)

#define PMAN_SEC_GROUP_MAX	SD_PMAN_NR_GRPS
#define PMAN_REGION_MAX		SD_PMAN_NR_RGNS

#define PTAB_MAGIC		"PST"

#define PTAB_RGN_MAX		(PMAN_SEC_GROUP_MAX * PMAN_REGION_MAX)

#ifdef SD_PST_ALIGN
# define PTAB_ALIGN		SD_PST_ALIGN
#else
# define PTAB_ALIGN		16
#endif

#define PTAB2PHYADDR(a)	((a) << 12)
#define PHY2PTABADDR(a)	(((a) >> 12) & ((1<<20) - 1))	/*in 4kB*/

#ifndef __ASSEMBLY__

/* alignment helpers */
#define LOCAL_ALIGNTO(x, a) ((x) & ~((a) - 1))
#define LOCAL_ALIGNTONEXT(x, a) (((x) + (a) - 1) & ~((a) - 1))

/**********************PMAN TABLE************************/

struct ptbl_rgn_body {
	union {
		struct { __extension__ uint32_t
			id: 3		/*pman id*/,
			hole0: 9	/*reserved*/,
			start: 20	/*start addr, in 4k granularity*/;
		} bits;
		uint32_t val;
	} lsb;
	uint32_t size;	/*size in bytes, must be multiple of 4k*/
	uint32_t sec;	/*security access*/
	uint32_t attr;	/*attributes*/
};

/*
 * PMAN TABLE DEFINITION
 *
--------+-----------------------+
        |  3  |  2  |  1  |  0  |
--------+-----+-----------------+------------
        |     |      magic      |
        | ver +-----+-----+-----+03~00H
   H    |     | 'T' | 'S' | 'P' |
   E    +-----+-----+-----------+------------
   A    |    dlen   |   tlen    |07~04H
   D    +-----------+-----------+------------
   E    |          dcrc         |0B~08H
   R    +-----------------------+------------
        |          hcrc         |0F~0CH
----+---+-----------------------+------------
    | r |          lsb          |13~10H
 R  | g |          size         |17~14H
    | n |          sec          |1B~18H
 E  | 0 |          attr         |1F~1CH
    +---+-----------------------+------------
 G  | r |          lsb          |23~20H
    | g |          size         |27~24H
 I  | n |          sec          |2B~28H
    | 1 |          attr         |2F~2CH
 O  +---+-----------------------+------------
    | . |          ...          |...
 N  +---+-----------------------+------------
    | r |          lsb          |10H*N+13~10H
 S  | g |          size         |10H*N+17~14H
    | n |          sec          |10H*N+1B~18H
    | N |          attr         |10H*N+1F~1CH
----+---+-----------------------+------------
 *
 *
 */
struct ptbl_hdr{
	char magic[3];		/*"PST"*/
	char ver;		/*version code*/
	uint16_t tlen;		/*table length, in bytes*/
	uint16_t dlen;		/*data length (region area), in bytes*/
	uint32_t dcrc;		/*checksum of regions*/
	uint32_t hcrc;		/*checksum of header*/
} __attribute__((packed));

#define _PTAB_HDR_TRACE(h, leading, _log) do{	\
	_log(leading "PMAN Secure Table:\n"	\
		"magic: %x %x %x\n"		\
		"ver:   0x%x\n"			\
		"tlen:  0x%x\n"			\
		"dlen:  0x%x\n"			\
		"dcrc:  0x%x\n"			\
		"hcrc:  0x%x\n",		\
		(h)->magic[0], (h)->magic[1],	\
		(h)->magic[2], (h)->ver,	\
		(h)->tlen, (h)->dlen,		\
		(h)->dcrc, (h)->hcrc);		\
}while(0)

#define _PTAB_REGION_TRACE(e, leading, _log) do{				\
	_log(leading "pman%d 0x%x 0x%x 0x%x 0x%x\n", 				\
	(e)->lsb.bits.id, (e)->lsb.bits.start, (e)->size, (e)->sec, (e)->attr);	\
}while(0)

#define REGION_ID(e) ((e)->lsb.bits.id)
#define REGION_START(e) ((e)->lsb.bits.start)
#define REGION_SIZE(e) ((e)->size)
#define REGION_SEC(e) ((e)->sec)
#define REGION_ATTR(e) ((e)->attr)

/*
 * return 0 on valid, non-zero otherwise
 */
static inline int region_sanity_chk_helper(const struct ptbl_rgn_body* e)
{
	int _ret = 0;
	_ret |= !(REGION_ID(e) >= 0 && REGION_ID(e) < PMAN_SEC_GROUP_MAX);/*id*/
	_ret |= !(REGION_SIZE(e) > 0); 				/*size*/
	_ret |= !(REGION_ATTR(e) & PMAN_RGN_ATTR_VALID); 	/*valid*/
	return _ret;
}

#define PTAB_REGION_SANITY_CHECK(e) region_sanity_chk_helper(e)

#define PTAB_REGION_SET(e, i) do{						\
	uint32_t _start = PTAB2PHYADDR((e)->lsb.bits.start);			\
	uint32_t _end = LOCAL_ALIGNTO((_start + (e)->size - 1), 		\
					LOG2_PMAN_REGION_GRANULARITY);		\
	trace_flow("==>pman%d[%d]: 0x%x 0x%x 0x%x 0x%x\n", 			\
	(e)->lsb.bits.id, i, _start, _end, (e)->sec, (e)->attr);		\
	pman_sec_region_set((e)->lsb.bits.id,i,_start,_end,(e)->sec,(e)->attr);	\
}while(0)

#define PTAB_REGION_INIT(e, id, start, sz, sec, attr) do{			\
	memset(e, 0, sizeof(struct ptbl_rgn_body));				\
	REGION_ID(e) = (id);							\
	REGION_START(e) = (start);						\
	REGION_SIZE(e) = (sz);							\
	REGION_SEC(e) = (sec);							\
	REGION_ATTR(e) = (attr);						\
}while(0)

/*
 * pman table length
 */
#define PTAB_LENGTH(t) ((t)->dlen + sizeof(struct ptbl_hdr))

/*
 * PTAB max length
 */
#define PTAB_MAX_LENGTH	(sizeof(struct ptbl_hdr) + sizeof(struct ptbl_rgn_body) * PTAB_RGN_MAX)

/*
 * pman table header length
 */
#define PTAB_HDR_LENGTH sizeof(struct ptbl_hdr)

/*
 * total of regions described in the table
 */
#define PTAB_TOTAL_REGIONS(t) ((t)->dlen / sizeof(struct ptbl_rgn_body))

/*
 * i-th region body in specified pman table
 */
#define PTAB_REGION(t, i) ((struct ptbl_rgn_body*)((t) + 1) + (i))

/*
 * iterate all regions in table
 */
#define FOR_EACH_PTAB_REGION(t, i) for((i)=0; (i)<PTAB_TOTAL_REGIONS(t); (i)++)

/*
 * number of regions of specified pman group described in the table
 */
static inline int number_of_regions_helper(struct ptbl_hdr *t, int id)
{
	int _i, _ret = 0;
	FOR_EACH_PTAB_REGION(t, _i) {
		if (REGION_ID(PTAB_REGION(t, _i)) == (id))
			_ret++;
	}
	return _ret;
}

#define PTAB_NUMBER_OF_REGIONS(t, id) number_of_regions_helper(t, id)

/*
 * void func(struct ptbl_rgn_body* rgn, void* param);
 *
 */
#define DO_FOR_EACH_PTAB_REGION(t, func, p) do{		\
	int _i;						\
	FOR_EACH_PTAB_REGION(t, _i) 			\
		func(PTAB_REGION(t, _i), p);		\
}while(0)

#endif /*__ASSEMBLY__*/
#endif /*__PMAN_SEC_TAB_H__*/
