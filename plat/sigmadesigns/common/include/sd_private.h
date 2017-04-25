/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SD_PRIVATE_H__
#define __SD_PRIVATE_H__

#include <sys/types.h>
#include <psci.h>
#include <sd_flash.h>
#include <sd_mcu_comm.h>
#include <sd_sip_svc.h>	/* MEM_STATE_XXX */

#define SYS_NOERROR	0
#define SYS_FAIL	1

#define SD_BOOT_NORMAL	0
#define SD_BOOT_RESUME	1

#ifndef FALSE
# define FALSE		0
#endif
#ifndef TRUE
# define TRUE		!FALSE
#endif

#ifndef __ASSEMBLY__

typedef unsigned long paddr_t;

#define SD_MAX(x, y) ((x) > (y) ? (x) : (y))
#define SD_MIN(x, y) ((x) < (y) ? (x) : (y))

/*
 * alignment must be power of 2
 */
#define SD_ALIGNTO(x, a) ((x) & ~((a) - 1))
#define SD_ALIGNTONEXT(x, a) (((x) + (a) - 1) & ~((a) - 1))

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

void sd_setup_page_tables(uintptr_t total_base, size_t total_size,
			  uintptr_t code_start, uintptr_t code_limit,
			  uintptr_t ro_start, uintptr_t ro_limit
#if USE_COHERENT_MEM
			  , uintptr_t coh_start, uintptr_t coh_limit
#endif
			  );

/* memory attributes */
enum {
	MEM_SEC = 0,
	MEM_NS,
	MEM_SRAM,
	MEM_FW,
	MEM_IO,
};
int sd_pbuf_is(const uint32_t attr, const paddr_t pa, const size_t len);

void* sd_phys_to_virt(const paddr_t pa);

#define ALIGNMENT_IS_OK(p, type)	\
	(((uintptr_t)(p) & (__alignof__(type) - 1)) == 0)

int sd_soc_pinshare_init_for_mmc(int id);
int32_t sd_soc_validate_power_state(unsigned int power_state,
				    psci_power_state_t *req_state);

unsigned int plat_sd_calc_core_pos(u_register_t mpidr);

void sd_timer_init(void);
void sd_timer_show_timestamp(void);

void sd_io_setup(void);
int sd_boot_load_raw_image(uintptr_t image_spec, uintptr_t image_base, size_t image_size);

int sd_ddr_init(void);

void plat_secondary_cold_boot_setup(void);
void sd_wakeup_secondary(uintptr_t entry, int core);
void sd_reset_mailbox(void);

int sd_relocate_mcu(void *img, unsigned int img_size);

/*
 * derive boot mode from mcu.
 * 0 - normal boot
 * 1 - resume from standby
 */
int sd_early_boot_mode(void);

/*
 * put all umac in access mode
 */
void sd_umac_access_mode(void);

/*
 * set training buffer for umacs
 */
void sd_umac_set_tr_area(void);

/*
 * interrupt
 */
void plat_sd_gic_driver_init(void);
void plat_sd_gic_init(void);
void plat_sd_gic_cpuif_enable(void);
void plat_sd_gic_cpuif_disable(void);
void plat_sd_gic_pcpu_init(void);

/*
 * security
 */
void plat_sd_security_setup(void);
void sd_dcsn_set_protections(void);
void sd_dcsn_drop_protections(void);
void sd_pman_set_protections(void);
void sd_pman_drop_protections(void);
void sd_soc_set_protections(void);

typedef struct _ddr_block {
	uintptr_t start;	/*inclusive*/
	uintptr_t end;		/*exclusive*/
}ddr_block_t;
/*
 * fn: int sd_soc_get_ddr_layout(ddr_block_t blobs[], int nb);
 * return number of umacs on success with address space of
 * each filled in array pointed by blobs, indexed by id.
 * Otherwise return error code (<0).
 */
int sd_soc_get_ddr_layout(ddr_block_t blobs[], int nb);

/*
 * PMAN calls error code
 */
enum {
	PMAN_E_OK = 0,
	PMAN_E_ERROR = -1,
	PMAN_E_INVAL = -2,
	PMAN_E_NOT_SUPPORT = -3,
};
/*
 * update pman security
 * give a chance to update pman protection settings from outside (deprecated)
 * <sz> bytes settings data shall be loaded to memory pointed by <tpa>
 * input params:
 * 	tva	- virtual address loaded with pman secure table, inclusive
 * 	sz	- pman secure table length, exclusive
 * return value:
 *	PMAN_OK on success. Otherwise error code.
 */
int sd_pman_update_protections(const uintptr_t tva, const size_t sz);

/*
 * check access state of specified memory range [pa, pa+sz)
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
int sd_pman_get_access_state(const paddr_t pa, const size_t sz);

/*
 * Board configure
 */
int sd_load_bdconf(void);
int sd_bc_boardid(void);
int sd_bc_paneltype(void);

//int dt_add_psci_node(void *fdt);
//int dt_add_psci_cpu_enable_methods(void *fdt);

#endif /*__ASSEMBLY__*/
#endif /*__SD_PRIVATE_H__*/
