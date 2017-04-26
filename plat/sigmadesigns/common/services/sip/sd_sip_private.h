/*****************************************
  Copyright 2017
  Sigma Designs, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/
/**
  @file   sd_sip_private.h
  @brief
	This file includes private declares for sigma designs defining sip services

  @author Tony He, tony_he@sigmadesigns.com
  @date   2017-4-24
  */

#ifndef __SD_SIP_PRIVATE_H__
#define __SD_SIP_PRIVATE_H__

#ifndef __ASSEMBLY__

int sd_sip_set_pst(const paddr_t pa, const size_t len);

int sd_sip_get_access_state(const paddr_t pa, const size_t len, uint32_t * const pout);

int sd_sip_mmio_read(const uint32_t mode, const paddr_t pa, unsigned long * const pout);

int sd_sip_mmio_write(const uint32_t mode, const paddr_t pa, const unsigned long val, const unsigned long mask);

int sd_sip_otp_read(const size_t ofs, const paddr_t pa, const size_t len, uint32_t *const pprot, const uint32_t ns);

int sd_sip_otp_write(const size_t ofs, const paddr_t pa, const size_t len, const uint32_t prot);

int sd_sip_get_rsa_pub_key(const paddr_t pa, const size_t len);

/*
 * Small helpers
 */
static inline paddr_t reg_pair_to_paddr(uint32_t reg0, uint32_t reg1)
{
	return (paddr_t)(((uint64_t)reg0 << 32) | reg1);
}

static inline void reg_pair_from_64(uint32_t *reg0, uint32_t *reg1, uint64_t val)
{
	*reg0 = val >> 32;
	*reg1 = val;
}

#endif /* !__ASSEMBLY__ */
#endif /* __SD_SIP_PRIVATE_H__ */
