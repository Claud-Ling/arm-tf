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

#define SYS_NOERROR	0
#define SYS_FAIL	1

#define SD_BOOT_NORMAL	0
#define SD_BOOT_RESUME	1

#ifndef __ASSEMBLY__

#define SD_MAX(x, y) ((x) > (y) ? (x) : (y))
#define SD_MIN(x, y) ((x) < (y) ? (x) : (y))

/*
 * alignment must be power of 2
 */
#define SD_ALIGNTO(x, a) ((x) & ~((a) - 1))
#define SD_ALIGNTONEXT(x, a) (((x) + (a) - 1) & ~((a) - 1))

void sd_configure_mmu_el1(unsigned long total_base, unsigned long total_size,
			  unsigned long ro_start, unsigned long ro_limit);

void sd_configure_mmu_el3(unsigned long total_base, unsigned long total_size,
			  unsigned long ro_start, unsigned long ro_limit);

int sd_soc_pinshare_init_for_mmc(int id);
int32_t sd_soc_validate_power_state(unsigned int power_state,
				    psci_power_state_t *req_state);

unsigned int plat_sd_calc_core_pos(u_register_t mpidr);

void sd_timer_init(void);
void sd_timer_show_timestamp(void);

void sd_io_setup(void);
int sd_boot_load_raw_image(uintptr_t image_spec, uintptr_t image_base, size_t image_size);

int sd_ddr_init(void);

void sd_wakeup_secondary(uintptr_t entry, int core);

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
