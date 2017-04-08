/*****************************************
  Copyright 2017
  Sigma Designs, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/
/**
  @file   pman_sec_private.h
  @brief
	This file decribes the privacy part of pman security register file for DTV chipset.


  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-03-31
  */

#include <pman_sec.h>

#ifndef __PMAN_SEC_PRIVATE_H__
#define __PMAN_SEC_PRIVATE_H__

#define PMAN_OK			0
#define PMAN_ERROR		1
#define PMAN_INVAL		2
#define PMAN_NOT_SUPPORT	3

#ifndef __ASSEMBLY__

DECLARE_PMAN_SEC_GROUPS;

/*
 * setup pman security
 */
int pman_set_protections(void);

#ifndef WITH_PROD
/*
 * drop pman security
 */
int pman_drop_protections(void);

/*
 * update pman security
 * give a chance to update pman protection settings from outside (deprecated)
 * <sz> bytes settings data shall be loaded to memory pointed by <tpa>
 * input params:
 * 	tpa	- physical address loaded with pman secure table, inclusive
 * 	sz	- pman secure table length, exclusive
 * return value:
 *	PMAN_OK on success. Otherwise error code.
 */
int pman_update_protections(const uint32_t tpa, const uint32_t sz);
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
int pman_get_group(const uint32_t pa, const uint32_t sz, uint32_t grps[], const uint32_t ng);

/*
 * check access state of specified memory range [pa, pa+len)
 * input params:
 * 	pa	- memory block start physical address, inclusive
 * 	sz	- memory block length, exclusive
 * return a bit wise value:
 *	bit[0]	- 1: secure accessible,   0: secure non-accessible
 *	bit[1]	- 1: non-secure readable, 0: non-secure non-readable
 *	bit[2]	- 1: non-secure writable, 0: non-secure non-writable
 *	bit[3]	- 1: secure executable,   0: secure non-executable
 *	bit[4]	- 1: ns executable,       0: non-secure non-executable
 *	others	- reserved, should be RAZ
 */
int pman_get_access_state(const uint32_t pa, const uint32_t sz);

#endif /*!__ASSEMBLY__*/
#endif /*__PMAN_SEC_PRIVATE_H__*/
