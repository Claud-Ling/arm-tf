/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
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

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <context.h>
#include <context_mgmt.h>
#include <debug.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <sd_private.h>

/* Macros to read the rk power domain state */
#define SD_CORE_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL0])
#define SD_CLUSTER_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL1])
#define SD_SYSTEM_PWR_STATE(state) \
	((state)->pwr_domain_state[PLAT_MAX_PWR_LVL])

extern uint64_t sd_sec_entry_point;
extern void plat_sd_warm_entrypoint(void);

/*
 * The following platform setup functions are weakly defined. They
 * provide typical implementations that will be overridden by a SoC.
 */
#pragma weak sd_soc_pwr_domain_suspend
#pragma weak sd_soc_pwr_domain_on
#pragma weak sd_soc_pwr_domain_off
#pragma weak sd_soc_pwr_domain_on_finish
#pragma weak sd_soc_prepare_system_reset

int sd_soc_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	return PSCI_E_NOT_SUPPORTED;
}

int sd_soc_pwr_domain_on(u_register_t mpidr)
{
	return PSCI_E_SUCCESS;
}

int sd_soc_pwr_domain_off(const psci_power_state_t *target_state)
{
	return PSCI_E_SUCCESS;
}

int sd_soc_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	return PSCI_E_SUCCESS;
}

int sd_soc_prepare_system_reset(void)
{
	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * This handler is called by the PSCI implementation during the `SYSTEM_SUSPEND`
 * call to get the `power_state` parameter. This allows the platform to encode
 * the appropriate State-ID field within the `power_state` parameter which can
 * be utilized in `pwr_domain_suspend()` to suspend to system affinity level.
******************************************************************************/
void sd_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	/* lower affinities use PLAT_MAX_OFF_STATE */
	for (int i = MPIDR_AFFLVL0; i < PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;

	/* max affinity uses system suspend state id */
	req_state->pwr_domain_state[PLAT_MAX_PWR_LVL] = PSTATE_ID_SOC_POWERDN;
}

/*******************************************************************************
 * Handler called when an affinity instance is about to enter standby.
 ******************************************************************************/
void sd_cpu_standby(plat_local_state_t cpu_state)
{
	assert(cpu_state == PLAT_MAX_RET_STATE);
	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
	dsb();
	wfi();
}

/*******************************************************************************
 * Handler called when an affinity instance is about to be turned on. The
 * level and mpidr determine the affinity instance.
 ******************************************************************************/
int sd_pwr_domain_on(u_register_t mpidr)
{
	assert(sd_sec_entry_point);
	sd_wakeup_secondary((uintptr_t)plat_sd_warm_entrypoint, MPIDR_AFFLVL0_VAL(mpidr));
	return sd_soc_pwr_domain_on(mpidr);
}

/*******************************************************************************
 * Handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void sd_pwr_domain_off(const psci_power_state_t *target_state)
{
	assert(SD_CORE_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE);

	plat_sd_gic_cpuif_disable();

	sd_soc_pwr_domain_off(target_state);
}

/*******************************************************************************
 * Handler called when called when a power domain is about to be suspended. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
void sd_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	sd_soc_pwr_domain_suspend(target_state);

	/* Prevent interrupts from spuriously waking up this cpu */
	plat_sd_gic_cpuif_disable();
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
void sd_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	//TODO: Shall we recover security settings right here?!

	/*
	 * Reset mailbox register
	 * This function is called with psci lock acquired so we are free from
	 * race condition.
	 */
	sd_reset_mailbox();

	/*
	 * Reset hardware settings.
	 */
	sd_soc_pwr_domain_on_finish(target_state);

	/* Enable the gic cpu interface */
	plat_sd_gic_pcpu_init();

	/* Program the gic per-cpu distributor or re-distributor interface */
	plat_sd_gic_cpuif_enable();
}

/*******************************************************************************
 * Handler called when a power domain has just been powered on after
 * having been suspended earlier. The target_state encodes the low power state
 * that each level has woken up from.
 ******************************************************************************/
void sd_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	sd_pwr_domain_on_finish(target_state);
	/*
	 * Program the gic per-cpu distributor or re-distributor interface.
	 */
	plat_sd_gic_cpuif_enable();
}

/*******************************************************************************
 * This is an optional function and, if implemented, is expected to perform
 * platform specific actions including the `wfi` invocation which allows the
 * CPU to powerdown. Since this function is invoked outside the PSCI locks,
 * the actions performed in this hook must be local to the CPU or the platform
 * must ensure that races between multiple CPUs cannot occur.

 * The `target_state` has a similar meaning as described in the `pwr_domain_off()`
 * operation and it encodes the platform coordinated target local power states for
 * the CPU power domain and its parent power domain levels. This function must
 * not return back to the caller.
 *
 * We use plat_secondary_cold_boot_setup to implement pwr_domain_pwr_down_wfi here
 * as the calling in core may not experience power lose before back to life, i.e.
 * cpu hotplug. This is because we don't have any core oriented pmu at all.
 ******************************************************************************/
__dead2 void sd_pwr_domain_pwr_down_wfi(const psci_power_state_t *target_state)
{
	/*
	 * drain the write buffer
	 */
	dsb();

	/*
	 * disable mmu and dcache as required by plat_secondary_cold_boot_setup
	 */
#ifdef AARCH32
	disable_mmu_secure();
#else
	disable_mmu_el3();
#endif

	/*
	 * Zzzzzz
	 * At this point mmu and dcache are both disabled, so we are ok to call
	 * plat_secondary_cold_boot_setup
	 */
	plat_secondary_cold_boot_setup();

	/*
	 * shall never reach here, lead to panic whatever
	 */
	plat_panic_handler();
}

/*******************************************************************************
 * Handler called when the system wants to be powered off
 ******************************************************************************/
__dead2 void sd_system_off(void)
{
	int ret;
	unsigned char dummy[4];
	NOTICE("Power off system\n");
	ret = mcomm_send_mcu_cmd(HOST_CMD_POWEROFF, (uintptr_t)dummy, sizeof(dummy));
	if (ret < 0) {
		ERROR("send poweroff cmd failed! error %d\n", ret);
		/* it usually means buggy when execution hits here.
		 * for mcomm driver shall already tried several times
		 * before return, it's no meaning to retry again.
		 */
	}
	/*
	 * trap the calling core
	 */
	do {
		wfi();
	} while (1);
}

/*******************************************************************************
 * Handler called when the system wants to be restarted.
 ******************************************************************************/
__dead2 void sd_system_reset(void)
{
	int ret;
	unsigned char dummy[4];
	/* per-SoC system reset handler */
	sd_soc_prepare_system_reset();

	/*
	 * Program the PMC in order to restart the system.
	 */
	NOTICE("Restart system\n");
	ret = mcomm_send_mcu_cmd(HOST_CMD_REBOOT, (uintptr_t)dummy, sizeof(dummy));
	if (ret < 0) {
		ERROR("send reboot cmd failed! error %d\n", ret);
		/* it usually means buggy when execution hits here.
		 * for mcomm driver shall already tried several times
		 * before return, it's no meaning to retry again.
		 */
	}
	/*
	 * trap the calling core
	 */
	do {
		wfi();
	} while (1);
}

/*******************************************************************************
 * Handler called to check the validity of the power state parameter.
 ******************************************************************************/
int32_t sd_validate_power_state(unsigned int power_state,
				   psci_power_state_t *req_state)
{
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);

	assert(req_state);

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	return sd_soc_validate_power_state(power_state, req_state);
}

/*******************************************************************************
 * Platform handler called to check the validity of the non secure entrypoint.
 ******************************************************************************/
int sd_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	if ((entrypoint >= SD_NS_DRAM_BASE) && (entrypoint <= (SD_NS_DRAM_BASE + SD_NS_DRAM_SIZE)))
		return PSCI_E_SUCCESS;

	return PSCI_E_INVALID_ADDRESS;
}

/*******************************************************************************
 * Export the platform handlers to enable psci to invoke them
 ******************************************************************************/
static const plat_psci_ops_t sd_plat_psci_ops = {
	.cpu_standby			= sd_cpu_standby,
	.pwr_domain_on			= sd_pwr_domain_on,
	.pwr_domain_off			= sd_pwr_domain_off,
	.pwr_domain_suspend		= sd_pwr_domain_suspend,
	.pwr_domain_on_finish		= sd_pwr_domain_on_finish,
	.pwr_domain_suspend_finish	= sd_pwr_domain_suspend_finish,
	.pwr_domain_pwr_down_wfi	= sd_pwr_domain_pwr_down_wfi,
	.system_off			= sd_system_off,
	.system_reset			= sd_system_reset,
	.validate_power_state		= sd_validate_power_state,
	.validate_ns_entrypoint		= sd_validate_ns_entrypoint,
	.get_sys_suspend_power_state	= sd_get_sys_suspend_power_state,
};

/*******************************************************************************
 * Export the platform specific power ops and initialize Power Controller
 ******************************************************************************/
int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	psci_power_state_t target_state = { { PSCI_LOCAL_STATE_RUN } };

	/*
	 * Flush entrypoint variable to PoC since it will be
	 * accessed after a reset with the caches turned off.
	 */
	sd_sec_entry_point = sec_entrypoint;
	flush_dcache_range((uint64_t)&sd_sec_entry_point, sizeof(uint64_t));

	/*
	 * Reset hardware settings.
	 */
	sd_soc_pwr_domain_on_finish(&target_state);

	/*
	 * Initialize PSCI ops struct
	 */
	*psci_ops = &sd_plat_psci_ops;

	return 0;
}
