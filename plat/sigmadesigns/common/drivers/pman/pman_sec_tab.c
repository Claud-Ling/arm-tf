/*****************************************
  Copyright 2017
  Sigma Designs, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/
/**
  @file   pman_sec_tab.c
  @brief  this file describes memory protections

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-04-03
  */

#include <bl_common.h>
#include <utils.h>
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <platform_def.h>
#include <sd_private.h>
#include <pmansetting.h>
#include "pman_sec_private.h"
#include "pman_sec_tab.h"

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 *****************************************************************************/
#pragma weak sd_soc_get_ddr_layout

/*
 * fn: int sd_soc_get_ddr_layout(ddr_block_t blobs[], int nb);
 * return number of umacs on success with address space of
 * each filled in array pointed by blobs, indexed by id.
 * Otherwise return error code (<0).
 */
int sd_soc_get_ddr_layout(ddr_block_t blobs[], const int nb)
{
	(void)blobs;
	(void)nb;
	return 0;
}

/********************************************************************************/
/* 	PMAN protection part 							*/
/********************************************************************************/

struct pman_mem_range {
	uint32_t addr;	/*physical addr*/
	uint32_t len;	/*byte length*/
};

struct pman_region_desc {
	uint8_t grp_id;
	uint8_t rgn_id;
#define RGN_SCORE_BEST		3
#define RGN_SCORE_INCLUDE	2
#define RGN_SCORE_PART		1
#define RGN_SCORE_EXCLUDE	0
#define RGN_SCORE_NOK		-1

	uint8_t score;	/*
			 * valid score values:
			 *   3	- best match mrange with region
			 *   2	- mrange is a subset of the region
			 *   1  - mrange partially falls the region
			 *   0	- mrange doesn't overlap with the region or invalid region
			 *   -1 - error
			 */
};

#define DESC2SECRGN(d) pman_sec_region((d)->grp_id, (d)->rgn_id)
#define DESC2TABRGN(t, d) PTAB_REGION(t, (d)->rgn_id)

#define PROT_STATE_OK		0
#define PROT_STATE_INVAL	1	/*invalid*/
#define PROT_STATE_ATTR_LESS	(1<<1)	/*attr not satisfied*/
#define PROT_STATE_ATTR_MORE	(1<<2)	/*extra attr is set*/
#define PROT_STATE_SEC_LESS	(1<<3)	/*sec not satisfied*/
#define PROT_STATE_SEC_MORE	(1<<4)	/*extra sec is set*/

/**********************PMAN TABLE************************/
#define trace_error(...) do{		\
	ERROR(__VA_ARGS__);		\
}while(0)

#define trace_dbg(...) do{		\
	INFO(__VA_ARGS__);		\
}while(0)

#define trace_flow(...) do{		\
	VERBOSE(__VA_ARGS__);		\
}while(0)

#define PTAB_HDR_TRACE(hdr, tag)	\
	_PTAB_HDR_TRACE(hdr, tag, trace_dbg)

#define PTAB_REGION_TRACE(e, leading)	\
	_PTAB_REGION_TRACE(e, leading, trace_dbg)

/*
 * validate checksum of PMAN table
 * return 1 on success, Otherwise zero
 * to be implemented...
 */
static int validate_checksum_helper(struct ptbl_hdr *tbl)
{
	return 1;
}

#define PTAB_VALIDATE_CHECKSUM(t) validate_checksum_helper(t)

struct ptab_rgn_statistics {
	uint32_t nrgn0;	/*number of regions of pman0*/
	uint32_t nrgn1;	/*number of regions of pman1*/
	uint32_t nrgn2;	/*number of regions of pman2*/
};


static void pman_get_secmem_range(struct pman_mem_range* range)
{
	memset(range, 0, sizeof(struct pman_mem_range));
	range->addr = SD_SEC_DRAM_BASE;
	range->len = SD_SEC_DRAM_SIZE;
}

static void pman_get_shmem_range(struct pman_mem_range* range)
{
	memset(range, 0, sizeof(struct pman_mem_range));
	range->addr = SD_WSM_BASE;
	range->len = SD_WSM_SIZE;
}

/*
 * check target memory addr against with specified region and return a score value
 *   3	- best match target with region
 *   2	- target is a subset of the region
 *   1  - target partially falls the region
 *   0	- target doesn't overlap with the region or invalid region
 *   -1 - error (N/A)
 */
static int pman_check_addr(struct pman_mem_range* rgn, struct pman_mem_range* target)
{
	int score = RGN_SCORE_EXCLUDE;
	if ((target->addr == rgn->addr) &&
		(target->len == rgn->len))
		score = RGN_SCORE_BEST;
	else if ((target->addr >= rgn->addr) &&
		(target->addr < rgn->addr + rgn->len) &&
		(target->len <= rgn->addr + rgn->len - target->addr))
		score = RGN_SCORE_INCLUDE;
	else if (((target->addr < rgn->addr) &&
		 (target->len > (rgn->addr - target->addr))) ||
		(target->addr < (rgn->addr + rgn->len) &&
		 target->len > (rgn->addr + rgn->len - target->addr)))
		score = RGN_SCORE_PART;
	else
		score = RGN_SCORE_EXCLUDE;

	trace_flow("rgn: [%x, %x], target: [%x, %x], score=%d\n", rgn->addr, rgn->len, target->addr, target->len, score);
	return score;
}

static void pman_rgn_invalidate(struct pman_sec_region *rgn, void* param)
{
	rgn->region_attr &= ~PMAN_RGN_ATTR_VALID;
}

/*
 * check memory addr with specified pman sec region and return a score value
 *   3	- best match mrange with region
 *   2	- mrange is a subset of the region
 *   1  - mrange partially falls the region
 *   0	- mrange doesn't overlap with the region or invalid region
 *   -1 - error
 */
static int pman_rgn_check_addr(struct pman_sec_region *rgn, void* param)
{
	int score = RGN_SCORE_EXCLUDE;
	struct pman_mem_range *mrange = (struct pman_mem_range*) param;
	struct pman_mem_range rgn_mem;
	memset(&rgn_mem, 0, sizeof(struct pman_mem_range));
	rgn_mem.addr = rgn->region_addr_low;
	rgn_mem.len = rgn->region_addr_high + PMAN_REGION_GRANULARITY - rgn->region_addr_low;
	if (!(rgn->region_attr & PMAN_RGN_ATTR_VALID))
		score = RGN_SCORE_EXCLUDE;
	else
		score = pman_check_addr(&rgn_mem, mrange);
	return score;
}

/*
 * determine if memory block [low, high) falls back to the default region
 * by searching all valid pman sec regions.
 * input params:
 * 	low	- memory block low address, inclusive
 * 	high	- memory block high address, exclusive
 * 	grp	- pman group id
 * 	idx	- pman sec region id
 * return value:
 * 	0	- all consumed by sec regions
 * 	1	- hit the default region
 */
static int pman_sec_match_default_region(const uint32_t low, const uint32_t high, const uint32_t grp, uint32_t idx)
{
	int state = 0;
	uint32_t rgn_low, rgn_high;

	/* all consumed */
	if (low == high)
		return 0;

	/* still some left when reach the end */
	if (idx > SD_PMAN_NR_RGNS - 1)
		return 1;

	/*
	 * assume valid regions are on top of invalid ones.
	 * still some left when come across invalid region.
	 */
	assert(grp < SD_PMAN_NR_GRPS);
	if (!pman_sec_region_is_valid(grp, idx))
		return 1;

	rgn_low  = pman_sec_region(grp, idx)->region_addr_low;	/* inclusive */
	rgn_high = pman_sec_region(grp, idx)->region_addr_high
			 + PMAN_REGION_GRANULARITY; /* exclusive */

	/* hit the best matched region */
	if (low >= rgn_low && high <= rgn_high)
		return 0;

	/* assume sec regions in ascending order */
	if (high < rgn_low)
		return 1;

	/* left part... */
	if (low < rgn_low)
		state = pman_sec_match_default_region(low, SD_MIN(high, rgn_low), grp, idx + 1);

	/* and right part if needed... */
	if (!state && high > rgn_high)
		state = pman_sec_match_default_region(SD_MAX(low, rgn_high), high, grp, idx + 1);

	return state;
}

/*
 * search pman sec regions of specified memory range
 * input params:
 * 	range	- specifies one memory range
 * 	descs	- pointer of region descriptor array
 * 	total	- number of elements of array
 * return value:
 * 	>0	- number of regions on success, with region descriptor filled in descs and number filled to *total
 * 	0	- no matched region
 * 	<0	- error (-1: address range check failed, -2: bad parameters)
 */
static int pman_sec_match_regions(struct pman_mem_range* range, struct pman_region_desc* descs, uint32_t* total)
{
	uint32_t num = 0, bad = 0;
	int score, grp = 0, idx = 0;

	if (descs == NULL || total == NULL || *total == 0)
		return -2;

	memset(descs, 0, sizeof(struct pman_region_desc) * (*total));
	for_each_sec_region(grp, idx) {
		if ((score = pman_rgn_check_addr(pman_sec_region(grp, idx), range)) > 0) {
			if (num >= *total) {
				trace_error("small buffer!\n");
				break;
			}
			descs[num].grp_id = grp;
			descs[num].rgn_id = idx;
			descs[num].score = score;
			num++;
		} else if (score < 0) {
			bad++;
			/*perhaps we still have chance, so don't break out here*/
		}
	}

	if (num == 0 && bad > 0)
		num = -1;

	*total = num;
	return num;
}

/*
 * perform sanity_check on memory protection
 * property: specified by access_golden and attr_golden
 * return value:
 * =0 	- ok
 * !=0	- failed
 */
static int pman_sanity_check_protection(const uint32_t access, const uint32_t attr, const uint32_t access_golden, const uint32_t attr_golden)
{
	int ret = PROT_STATE_OK;
	/*valid?*/
	if (!(attr & PMAN_RGN_ATTR_VALID)) {
		trace_dbg("invalid region!\n");
		return PROT_STATE_INVAL;
	}

	/*attr*/
	if ((attr & attr_golden) != attr_golden) {
		trace_flow("attr is not satisfied (0x%x vs 0x%x)\n", attr, attr_golden);
		ret |= PROT_STATE_ATTR_LESS;
	}
	if (attr & (~(attr_golden | PMAN_RGN_ATTR_VALID))) {
		trace_flow("extra attr is set other than required ones (0x%x vs 0x%x)\n",
			attr, attr_golden);
		ret |= PROT_STATE_ATTR_MORE;
	}

	/*sec access*/
	if ((access & access_golden) != access_golden) {
		trace_flow("secure access is not satisfied (0x%x vs 0x%x)\n",
			access, access_golden);
		ret |= PROT_STATE_SEC_LESS;
	}
	if (access & (~access_golden)) {
		trace_flow("accessible to targets other than required ones (0x%x vs 0x%x)\n",
			access, access_golden);
		ret |= PROT_STATE_SEC_MORE;
	}

	return ret;
}

static int sanity_check_protection_helper(uint32_t acc, uint32_t attr, uint32_t acc_golden, uint32_t attr_golden, uint32_t accepts)
{
	uint32_t ret;
	ret = pman_sanity_check_protection(acc, attr, acc_golden, attr_golden);
	return (((ret == PROT_STATE_OK) || !(ret & ~accepts)) ? 0 : 1);
}

/*
 * perform sanity_check on sec memory protection
 * property: r/w/x, a9_secure/turing/crypto only
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_secmem(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_SEC | PMAN_RGN_SEC_TURING | PMAN_RGN_SEC_CRYPTO),		\
	(PMAN_RGN_ATTR_READ | PMAN_RGN_ATTR_WRITE | PMAN_RGN_ATTR_EXEC),		\
	PROT_STATE_ATTR_MORE)

/*
 * perform sanity_check on share memory protection
 * property: r/w, a9/turing/crypto
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_shmem(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_SEC | PMAN_RGN_SEC_ARM_NS | PMAN_RGN_SEC_TURING | PMAN_RGN_SEC_CRYPTO),	\
	(PMAN_RGN_ATTR_READ | PMAN_RGN_ATTR_WRITE | PMAN_RGN_ATTR_EXEC),		\
	(PROT_STATE_ATTR_MORE | PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on secure r/w memory
 * property: r/w, a9_secure/turing/crypto, don't care others
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_srw(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_SEC | PMAN_RGN_SEC_TURING | PMAN_RGN_SEC_CRYPTO),		\
	(PMAN_RGN_ATTR_READ | PMAN_RGN_ATTR_WRITE),					\
	(PROT_STATE_ATTR_MORE | PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on nonsecure write only memory
 * property: write only from non-secure world, don't care secure world
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_nwo(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_NS),								\
	(PMAN_RGN_ATTR_WRITE),								\
	(PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on nonsecure read only memory
 * property: read only from non-secure world, don't care secure world
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_nro(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_NS),								\
	(PMAN_RGN_ATTR_READ),								\
	(PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on nonsecure writable memory
 * property: writable from non-secure world, don't care secure world and r/x
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_nw(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_NS),								\
	(PMAN_RGN_ATTR_WRITE),								\
	(PROT_STATE_ATTR_MORE | PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on nonsecure readable memory
 * property: readable from non-secure world, don't care secure world and w/x
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_nr(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_NS),								\
	(PMAN_RGN_ATTR_READ),								\
	(PROT_STATE_ATTR_MORE | PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on secure executable memory
 * property: executable from secure world, don't care nonsecure world and r/w
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_sx(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_SEC),								\
	(PMAN_RGN_ATTR_EXEC),								\
	(PROT_STATE_ATTR_MORE | PROT_STATE_SEC_MORE))

/*
 * perform sanity_check on nonsecure executable memory
 * property: executable from nonsecure world, don't care secure world and r/w
 * return value:
 * 0 	- ok
 * 1	- failed
 */
#define pman_sanity_check_mem_nsx(access, attr)						\
	sanity_check_protection_helper(access, attr,					\
	(PMAN_RGN_SEC_ARM_NS),								\
	(PMAN_RGN_ATTR_EXEC),								\
	(PROT_STATE_ATTR_MORE | PROT_STATE_SEC_MORE))

static void ptab_region_statistics(struct ptbl_rgn_body* rgn, void* param)
{
	struct ptab_rgn_statistics *s = (struct ptab_rgn_statistics*) param;

	if (PTAB_REGION_SANITY_CHECK(rgn))
		return;	/*don't count on invalid one*/

	if (REGION_ID(rgn) == 0)
		s->nrgn0++;
	else if (REGION_ID(rgn) == 1)
		s->nrgn1++;
	else if (REGION_ID(rgn) == 2)
		s->nrgn2++;
}

/*
 * check memory addr with specified pman tbl region and return a score value
 *   3	- best match mrange with region
 *   2	- mrange is a subset of the region
 *   1  - mrange partially falls the region
 *   0	- mrange doesn't overlap with the region or invalid region
 *   -1 - error
 */
static int ptbl_region_check_addr(struct ptbl_rgn_body* rgn, void* param)
{
	int score = RGN_SCORE_EXCLUDE;
	struct pman_mem_range *mrange = (struct pman_mem_range*) param;
	struct pman_mem_range rgn_mem;
	memset(&rgn_mem, 0, sizeof(struct pman_mem_range));
	rgn_mem.addr = PTAB2PHYADDR(rgn->lsb.bits.start);
	rgn_mem.len = rgn->size;
	if (PTAB_REGION_SANITY_CHECK(rgn))
		score = RGN_SCORE_EXCLUDE;
	else
		score = pman_check_addr(&rgn_mem, mrange);
	return score;
}

/*
 * search pman tab regions of specified memory range
 * input params:
 * 	range	- specifies one memory range
 * 	descs	- poiter of one region descriptor array
 * 	total	- number of elements of array
 * return value:
 * 	>0	- number of regions on success, with region descriptor filled in descs and number filled to *total
 * 	0	- use default range
 * 	<0	- error (-1: address range check failed, -2: bad parameters)
 */
static int pman_tab_match_regions(struct pman_mem_range* range, struct pman_region_desc* descs, uint32_t* total, struct ptbl_hdr *tbl)
{
	uint32_t num = 0, bad = 0;
	int score, idx = 0;

	if (range == NULL || descs == NULL || total == NULL || *total == 0 || tbl == NULL)
		return -2;

	FOR_EACH_PTAB_REGION(tbl, idx) {
		if ((score = ptbl_region_check_addr(PTAB_REGION(tbl, idx), range)) > 0) {
			if (num >= *total) {
				trace_error("small buffer!\n");
				break;
			}
			descs[num].grp_id = REGION_ID(PTAB_REGION(tbl, idx));
			descs[num].rgn_id = idx;
			descs[num].score = score;
			num++;
		} else if (score < 0) {
			bad++;
			/*perhaps we still have chance, so don't break out here*/
		}
	}

	if (num == 0 && bad > 0)
		num = -1;

	*total = num;
	return num;
}

/*
 * deploy all valid memory protections from specified table
 */
static int pman_deploy_table(const struct ptbl_hdr *tbl)
{
	int i, j, pos = 0, grp = -1;

	if (tbl == NULL)
		return -1;

	FOR_EACH_PTAB_REGION(tbl, i){
		if (PTAB_REGION_SANITY_CHECK(PTAB_REGION(tbl, i))) {
			PTAB_REGION_TRACE(PTAB_REGION(tbl, i), "Skip invalid region:");
			continue;
		}

		if (REGION_ID(PTAB_REGION(tbl, i)) != grp) {
			grp = REGION_ID(PTAB_REGION(tbl, i));
			pos = 0;
		}

		for_each_sec_region_of_group_from_pos(grp, j, pos) {
			if (!pman_sec_region_is_valid(grp, j)) {
				PTAB_REGION_SET(PTAB_REGION(tbl, i), j);
				pos = j + 1;	/*next*/
				break;
			}
		}

		if (j == PMAN_REGION_MAX) {
			PTAB_REGION_TRACE(PTAB_REGION(tbl, i), "No seat for region:");
			continue;
		}
	}
	return 0;
}

static int pman_sec_setup(const uintptr_t tpa, const int32_t len)
{
	int ret = PMAN_ERROR;
	struct pman_mem_range mrange;
	struct pman_region_desc rgns[SD_PMAN_NR_RGNS];
	const uint32_t total = ARRAY_SIZE(rgns);
	uint32_t num;

	/*any pass-in security settings?*/
	if (tpa != 0 && len != 0) {
#define PTAB_HANDLE_ERROR(...) do {			\
		trace_error(__VA_ARGS__);		\
		ret = PMAN_ERROR;			\
		if (tbl != NULL) {			\
			tbl = NULL;			\
		}					\
		goto OUT;				\
}while(0)
		struct ptab_rgn_statistics stat;
		struct ptbl_hdr *tbl = NULL;
		trace_dbg("get tbl, pa: 0x%lx, length:0x%x\n", tpa, len);
		/*
		 * Validate input parameters.
		 */
		if (tpa == 0 || len < sizeof(struct ptbl_hdr)) {
			return PMAN_INVAL;
		}

		tbl = (struct ptbl_hdr *)tpa;	/* atf uses idmap */
		PTAB_HDR_TRACE(tbl,"");
		/*Validate pman table*/
		if (strncmp(tbl->magic, PTAB_MAGIC, strlen(PTAB_MAGIC)) ||
			tbl->tlen > len ||
			!PTAB_VALIDATE_CHECKSUM(tbl)) {
			PTAB_HANDLE_ERROR("invalid PST!\n");
		}

		if (tbl->ver > PST_VERSION_CODE) {
			PTAB_HANDLE_ERROR("unsupported PST\n");
		} else if (tbl->ver < PST_VERSION_CODE) {
			WARN("legacy PST is found\n");
		}

		memset(&stat, 0, sizeof(struct ptab_rgn_statistics));
		DO_FOR_EACH_PTAB_REGION(tbl, ptab_region_statistics, &stat);
		if (stat.nrgn0 > PMAN_REGION_MAX || stat.nrgn1 > PMAN_REGION_MAX || stat.nrgn2 > PMAN_REGION_MAX) {
			PTAB_HANDLE_ERROR("number of regions exceeds %d per group!\n", PMAN_REGION_MAX);
		}

		/*
		 * Sanity check on pass-in region settings
		 * 1. sec memory shall not be accessible from NS world
		 * 2. share memory r/w to both Secure and NS world
		 * 3. a/v buffer shall be write only to NS world
		 */
		/* sec mem: r/w/x to secure world but none to NS world */
		pman_get_secmem_range(&mrange);
		num = total;
		num = pman_tab_match_regions(&mrange, rgns, &num, tbl);
		if (num == 0) {
			//none entry
			PTAB_HANDLE_ERROR("secmem: missed in tab!\n");
		} else if (num == 1 && rgns[0].score == RGN_SCORE_BEST) {
			//one best matched region
			if (pman_sanity_check_secmem(DESC2TABRGN(tbl, &rgns[0])->sec, DESC2TABRGN(tbl, &rgns[0])->attr)) {
				PTAB_HANDLE_ERROR("secmem: sanity check failed!\n");
			}
		} else if (num > 1){
			//multiple regions
			PTAB_HANDLE_ERROR("secmem: multi regions in tab!\n");
		} else {
			//NOK
			PTAB_HANDLE_ERROR("secmem: other error!\n");
		}

		/*sharemem: r/w to both secure and NS world*/
		pman_get_shmem_range(&mrange);
		num = total;
		num = pman_tab_match_regions(&mrange, rgns, &num, tbl);
		if (num == 0) {
			//none entry
			PTAB_HANDLE_ERROR("shmem: missed in tab!\n");
		} else if (num == 1 && rgns[0].score == RGN_SCORE_BEST) {
			//one best matched region
			if (pman_sanity_check_shmem(DESC2TABRGN(tbl, &rgns[0])->sec, DESC2TABRGN(tbl, &rgns[0])->attr)) {
				PTAB_HANDLE_ERROR("shmem: sanity check failed!\n");
			}
		} else if (num > 1){
			//multiple regions
			PTAB_HANDLE_ERROR("shmem: multi regions in tab!\n");
		} else {
			//NOK
			PTAB_HANDLE_ERROR("shmem: other error!\n");
		}

		/*a/v buffer: write only to NS world*/
		//TODO

		/*invalidate all sec regions first*/
		do_for_each_sec_region(pman_rgn_invalidate, NULL);
		/*deploy settings*/
		pman_deploy_table(tbl);
		/*unmap*/
		tbl = NULL;
#undef PTAB_HANDLE_ERROR
		ret = PMAN_OK;
	} /*if (ta != 0)*/

OUT:
	return ret;
}

int pman_set_protections(void)
{
	uintptr_t tpa;
	uint32_t sz;
	tpa = (uintptr_t)pman_table;
	sz = ARRAY_SIZE(pman_table);
	return pman_sec_setup(tpa, sz);
}

#ifndef WITH_PROD
/*
 * warning: drop memory protection from sec, open sec memory to NS world!!
 */
int pman_drop_protections(void)
{
	struct pman_mem_range mrange;
	struct pman_region_desc rgns[SD_PMAN_NR_RGNS];
	const uint32_t total = ARRAY_SIZE(rgns);
	uint32_t num;

	pman_get_secmem_range(&mrange);
	num = total;
	num = pman_sec_match_regions(&mrange, rgns, &num);
	if (num == 1 && rgns[0].score == RGN_SCORE_BEST) {
		/*open secmem to NS world*/
		DESC2SECRGN(&rgns[0])->region_sec_access |= PMAN_RGN_SEC_ARM_NS;
		return PMAN_OK;
	} else {
		trace_dbg("failed: wrong region allocation for sec memory!\n"); /*shall not happen*/
		return PMAN_ERROR;
	}
}

/*
 * update pman security
 * give a chance to update pman protection settings from outside (deprecated)
 * <sz> bytes settings data shall be loaded to memory pointed by <tpa>
 */
int pman_update_protections(const uint32_t tpa, const uint32_t sz)
{
	if (tpa == 0 || sz == 0)
		return PMAN_INVAL;
	else
		return pman_sec_setup(tpa, sz);
}
#endif

/*
 * determine pman sec groups of specified memory block
 * input params:
 * 	pa	- memory block start physical address, inclusive
 * 	sz	- memory block length, exclusive
 * 	grps	- pointer of group id array
 * 	ng	- number of elements in array
 * return value:
 * 	>0	- number of groups on success, with group id filled in array pointed by grps.
 * 	0	- none matched pman group.
 * 	<0	- error (-1: small buffer).
 */
int pman_get_group(const uint32_t pa, const uint32_t sz, uint32_t grps[], const uint32_t ng)
{
	int i, nb = 0, cnt = 0;
	ddr_block_t blobs[SD_PMAN_NR_GRPS];
	nb = sd_soc_get_ddr_layout(blobs, ARRAY_SIZE(blobs));
	for (i = 0; i < nb; i++) {
		if ((pa >= blobs[i].start && pa < blobs[i].end) ||
		    ((pa + sz) > blobs[i].start && (pa + sz) <= blobs[i].end)) {
			if (cnt < ng) {
				grps[cnt++] = i;
			} else {
				trace_error("small buffer!\n");
				return -1;
			}
		}
	}
	return cnt;
}

/*
 * check access state of specified memory range [pa, pa+len)
 * return a bit wise value:
 *	bit[0]	- 1: secure accessible,   0: secure non-accessible
 *	bit[1]	- 1: non-secure readable, 0: non-secure non-readable
 *	bit[2]	- 1: non-secure writable, 0: non-secure non-writable
 *	bit[3]	- 1: secure executable,   0: secure non-executable
 *	bit[4]	- 1: ns executable,       0: non-secure non-executable
 *	others	- reserved, should be RAZ
 */
int pman_get_access_state(const uint32_t pa, const uint32_t sz)
{
	struct pman_mem_range mrange;
	struct pman_region_desc rgns[SD_PMAN_NR_RGNS];
	ddr_block_t blobs[SD_PMAN_NR_GRPS];
	uint32_t num;
	int i, state = 0;

	if (0 == sz) return 0;

	memset(&mrange, 0, sizeof(struct pman_mem_range));
	mrange.addr = pa;
	mrange.len = sz;
	num = ARRAY_SIZE(rgns);
	num = pman_sec_match_regions(&mrange, rgns, &num);
	if (num > 0) {
		//hit one or more regions
		for (i=0; i<num; i++) {
			if (!pman_sanity_check_mem_srw(DESC2SECRGN(&rgns[i])->region_sec_access, DESC2SECRGN(&rgns[i])->region_attr))
				state |= MEM_STATE_S_RW;
			if (!pman_sanity_check_mem_nr(DESC2SECRGN(&rgns[i])->region_sec_access, DESC2SECRGN(&rgns[i])->region_attr))
				state |= MEM_STATE_NS_RD;
			if (!pman_sanity_check_mem_nw(DESC2SECRGN(&rgns[i])->region_sec_access, DESC2SECRGN(&rgns[i])->region_attr))
				state |= MEM_STATE_NS_WR;
			if (!pman_sanity_check_mem_sx(DESC2SECRGN(&rgns[i])->region_sec_access, DESC2SECRGN(&rgns[i])->region_attr))
				state |= MEM_STATE_S_EXEC;
			if (!pman_sanity_check_mem_nsx(DESC2SECRGN(&rgns[i])->region_sec_access, DESC2SECRGN(&rgns[i])->region_attr))
				state |= MEM_STATE_NS_EXEC;
		}
	} else if (num < 0){
		//NOK
		trace_error("fail to match region [%x, %x)!\n", pa, pa + sz);
	}

	num = sd_soc_get_ddr_layout(blobs, ARRAY_SIZE(blobs));
	for (i = 0; i < num; i++) {
		if (((pa >= blobs[i].start && pa < blobs[i].end) ||
		    ((pa + sz) > blobs[i].start && (pa + sz) <= blobs[i].end)) &&
		    pman_sec_match_default_region(pa, pa + sz, i, 0)) {
			//fallback to the default region
			trace_dbg("fall back to pman%d default region [%x,%x)\n", i, pa, pa+sz);
			if (pman_sec_dft_read_is_secure(i) &&
			    pman_sec_dft_write_is_secure(i))
				state |= MEM_STATE_S_RW;
			if (pman_sec_dft_read_is_nonsecure(i))
				state |= MEM_STATE_NS_RD;
			if (pman_sec_dft_write_is_nonsecure(i))
				state |= MEM_STATE_NS_WR;
			if (pman_sec_dft_exec_is_secure(i))
				state |= MEM_STATE_S_EXEC;
			if (pman_sec_dft_exec_is_nonsecure(i))
				state |= MEM_STATE_NS_EXEC;
		}
	}

	return state;
}

