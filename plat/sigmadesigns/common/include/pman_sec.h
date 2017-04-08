/*****************************************
  Copyright 2016
  Sigma Designs, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/
/**
  @file   pman_sec.h
  @brief
	This file decribes pman security register file for DTV chipset.

	The host shall define follow macros before include this file:
	PMAN_SEC_GROUP_MAX	- number of security group
	PMAN_REGION_MAX		- number of regions in one group

  @author Tony He, tony_he@sigmadesigns.com
  @date   2016-11-29
  */

#include <sys/types.h>
#include <debug.h>
#include <pman_security_no.h>

#ifndef __PMAN_SEC_H__
#define __PMAN_SEC_H__

/*
 * attributes
 */
#define PMAN_RGN_ATTR_WRITE	(1 << 16)
#define PMAN_RGN_ATTR_EXEC	(1 << 12)
#define PMAN_RGN_ATTR_READ	(1 << 8)
#define PMAN_RGN_ATTR_CONTENT	(1 << 4)
#define PMAN_RGN_ATTR_VALID	(1 << 0)

#define LOG2_PMAN_REGION_GRANULARITY	12
#define PMAN_REGION_GRANULARITY		(1 << LOG2_PMAN_REGION_GRANULARITY)

#include <pman_sec_tab.h>

#ifndef __ASSEMBLY__

struct pman_sec {
	volatile uint32_t addr_mask;		/* +0x00000000 */
	volatile uint32_t hole0[(0x20 - 4) >> 2];
	volatile uint32_t blocked_cmd;		/* +0x00000020 */
	volatile uint32_t blocked_cmd_id;
	volatile uint32_t blocked_cmd_bs;
	volatile uint32_t blocked_cmd_lines;
	volatile uint32_t blocked_cmd_stride;
	volatile uint32_t blocked_cmd_addr;
	volatile uint32_t blocked_sec_group;
	volatile uint32_t blocked_region;
	volatile uint32_t hole1[(0x60 - 0x40) >> 2];
	volatile uint32_t def_instr_sec_access;	/* +0x00000060 */
	volatile uint32_t def_read_sec_access;
	volatile uint32_t def_write_sec_access;
	volatile uint32_t hole2[(0x100 - 0x6c) >> 2];
	struct pman_sec_region {		/* +0x00000100+n*0x20 */
		volatile uint32_t region_addr_low;	/*inclusive*/
		volatile uint32_t region_addr_high;	/*inclusive*/
		volatile uint32_t region_sec_access;
		volatile uint32_t region_attr;
		volatile uint32_t hole3[4];
	} regions[PMAN_REGION_MAX];
	//volatile uint32_t hole4[(0x400 - PMAN_REGION_MAX * 0x20) >> 2];
	volatile uint32_t init_err_rd_data[6];	/* +0x00000500+n*0x4 */
	volatile uint32_t hole5[(0xfd8 - 0x518) >> 2];
	volatile uint32_t int_clr_enable;	/* +0x00000fd8 */
	volatile uint32_t int_set_enable;
	volatile uint32_t int_status;
	volatile uint32_t int_enable;
	volatile uint32_t int_clr_status;
	volatile uint32_t int_set_status;
	volatile uint32_t hole6[(0xffc - 0xff0) >> 2];
	volatile uint32_t module_id;		/* +0x00000ffc */
};

#define DEFINE_PMAN_SEC_GROUPS(...)					\
struct pman_sec *pPMAN_SEC_groups[PMAN_SEC_GROUP_MAX] = {__VA_ARGS__}

#define DECLARE_PMAN_SEC_GROUPS						\
extern struct pman_sec *pPMAN_SEC_groups[PMAN_SEC_GROUP_MAX]

#ifdef PMAN_SEC_DEBUG
# define pman_sec_sanity_chkgrp(g) do{					\
	if ((g)<0 || (g)>PMAN_SEC_GROUP_MAX-1) {			\
		ERROR("invalid sec_group:%d\n", g);			\
		panic();						\
	}								\
}while(0)

# define pman_sec_sanity_chkrgn(r) do{					\
	if ((r)<0 || (r)>PMAN_REGION_MAX-1) {				\
		ERROR("invalid sec_region:%d\n", r);			\
		panic();						\
	}								\
}while(0)

#define pman_sec_obj(grp) ({						\
	pman_sec_sanity_chkgrp(grp);					\
	pPMAN_SEC_groups[grp];						\
})
#define pman_sec_region(grp, idx) ({					\
	pman_sec_sanity_chkgrp(grp);					\
	pman_sec_sanity_chkrgn(idx);					\
	&pPMAN_SEC_groups[grp] -> regions[idx];				\
})

#else
# define pman_sec_sanity_chkgrp(g) do{}while(0)
# define pman_sec_sanity_chkrgn(g) do{}while(0)
#define pman_sec_obj(grp) pPMAN_SEC_groups[grp]
#define pman_sec_region(grp, idx) (&pPMAN_SEC_groups[grp] -> regions[idx])

#endif

#define pman_sec_region_is_valid(grp, idx) (pman_sec_region(grp, idx)->region_attr & PMAN_RGN_ATTR_VALID)
#define pman_sec_region_is_writable(grp, idx) (pman_sec_region_is_valid(grp, idx) &&	\
		(pman_sec_region(grp, idx)->region_attr & PMAN_RGN_ATTR_WRITE))
#define pman_sec_region_is_executable(grp, idx) (pman_sec_region_is_valid(grp, idx) &&	\
		(pman_sec_region(grp, idx)->region_attr & PMAN_RGN_ATTR_EXEC))
#define pman_sec_region_is_readable(grp, idx) (pman_sec_region_is_valid(grp, idx) &&	\
		(pman_sec_region(grp, idx)->region_attr & PMAN_RGN_ATTR_READ))


#define pman_sec_is_secure(access) ((access) & PMAN_RGN_SEC_ARM_SEC)
#define pman_sec_is_nonsecure(access) ((access) & PMAN_RGN_SEC_ARM_NS)
#define pman_sec_region_is_secure(grp, idx) (pman_sec_region_is_valid(grp, idx) &&	\
		(pman_sec_is_secure(pman_sec_region(grp, idx)->region_sec_access)))
#define pman_sec_region_is_nonsecure(grp, idx) (pman_sec_region_is_valid(grp, idx) &&	\
		(pman_sec_is_nonsecure(pman_sec_region(grp, idx)->region_sec_access)))
#define pman_sec_region_check(grp, idx, m) (pman_sec_region_is_valid(grp, idx) &&	\
		((pman_sec_region(grp, idx)->region_sec_access & (m)) == (m)))

#define pman_sec_dft_exec_is_secure(grp) (pman_sec_obj(grp)->def_instr_sec_access & PMAN_RGN_SEC_ARM_SEC)
#define pman_sec_dft_exec_is_nonsecure(grp) (pman_sec_obj(grp)->def_instr_sec_access & PMAN_RGN_SEC_ARM_NS)
#define pman_sec_dft_read_is_secure(grp) (pman_sec_obj(grp)->def_read_sec_access & PMAN_RGN_SEC_ARM_SEC)
#define pman_sec_dft_read_is_nonsecure(grp) (pman_sec_obj(grp)->def_read_sec_access & PMAN_RGN_SEC_ARM_NS)
#define pman_sec_dft_write_is_secure(grp) (pman_sec_obj(grp)->def_write_sec_access & PMAN_RGN_SEC_ARM_SEC)
#define pman_sec_dft_write_is_nonsecure(grp) (pman_sec_obj(grp)->def_write_sec_access & PMAN_RGN_SEC_ARM_NS)

#define pman_sec_region_length(g, r) (pman_sec_region_is_valid(g, r) ? (pman_sec_region(g, r)->region_addr_high + PMAN_REGION_GRANULARITY - pman_sec_region(g, r)->region_addr_low) : 0)

#define pman_sec_region_invalidate(grp, idx) do{			\
	pman_sec_region(grp, idx)->region_attr &= ~PMAN_RGN_ATTR_VALID;	\
}while(0)

#define pman_sec_region_set(grp, idx, low, high, sec, attr) do{		\
	pman_sec_region(grp, idx)->region_addr_low = (low);		\
	pman_sec_region(grp, idx)->region_addr_high = (high);		\
	pman_sec_region(grp, idx)->region_sec_access = (sec);		\
	pman_sec_region(grp, idx)->region_attr = (attr);		\
}while(0)

/*
 * iterate over a regions array starting from pos of specified pman sec group
 * @param[in]   g      the id of your sec group
 * @param[in]   idx    the index of regions array to use as a loop counter
 * @param[in]   pos    the index of start region,  which indicates loop start is pman_sec.regions[pos]
 * while pos should be: 0 <= pos < PMAN_REGION_MAX
 */
#define for_each_sec_region_of_group_from_pos(g, idx, pos)		\
	for ((idx)=(pos); (idx)<PMAN_REGION_MAX; (idx)++)

/*
 * iterate over a regions array of specified pman sec group
 * @param[in]   g      the id of your sec group
 * @param[in]   idx    the index of regions array to use as a loop counter
 */
#define for_each_sec_region_of_group(g, idx)				\
	for_each_sec_region_of_group_from_pos(g, idx, 0)

/*
 * iterate over regions arrays of all pman sec groups
 * @param[in]   g      the id of your sec group to use as first level loop counter
 * @param[in]   idx    the index of a regions array to use as a second level loop counter
 */
#define for_each_sec_region(g, idx)					\
	for ((g)=0; (g)<PMAN_SEC_GROUP_MAX; (g)++)			\
		for_each_sec_region_of_group(g, idx)

/*
 * iterate over a regions array of specified pman sec group, and execute func on each region.
 * the prototype of func is:  void func(struct pman_sec_region *rgn, void* param);
 * @param[in]   g      the id of your sec group
 * @param[in]   func   an function pointer
 * @param[in]   v      the second parameter of func to use as a func private data
 */
#define do_for_each_sec_region_of_group(g, func, v) do{			\
	int _i = 0;							\
	for_each_sec_region_of_group(g, _i)				\
		func(pman_sec_region(g, _i), v);			\
}while(0)

/*
 * iterate over regions arrays of all pman sec groups, and execute func on each region.
 * the prototype of func is:  void func(struct pman_sec_region *rgn, void* param);
 * @param[in]   func   an function pointer
 * @param[in]   v      the second parameter of func to use as a func private data
 */
#define do_for_each_sec_region(func, v) do{				\
	int _g, _i = 0;							\
	for_each_sec_region(_g, _i)					\
		func(pman_sec_region(_g, _i), v);			\
}while(0)

#endif /*!__ASSEMBLY__*/
#endif /*__PMAN_SEC_H__*/
