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

#include <arch.h>
#include <asm_macros.S>
#include <sd_def.h>

	.globl	plat_is_my_cpu_primary
	.globl	plat_my_core_pos
	.globl	plat_sd_calc_core_pos
	.globl	plat_get_my_entrypoint
	.globl	plat_secondary_cold_boot_setup
	.globl	platform_mem_init
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_report_exception
	.globl	plat_reset_handler
	.globl	plat_sd_warm_entrypoint
	.globl	sd_sec_entry_point
	.globl	ns_image_entrypoint

	/* -----------------------------------------------------
	 * unsigned int plat_is_my_cpu_primary(void);
	 *
	 * This function checks if this is the Primary CPU
	 * -----------------------------------------------------
	 */
func plat_is_my_cpu_primary
	mrs	x0, mpidr_el1
	ands	x0, x0, #(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK)
	cset	x0, eq
	ret
endfunc plat_is_my_cpu_primary

	/* -----------------------------------------------------
	 * unsigned int plat_my_core_pos(void);
	 *
	 * result: CorePos = CoreId + (ClusterId << 2)
	 * -----------------------------------------------------
	 */
func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_sd_calc_core_pos
endfunc plat_my_core_pos

/*
 *  unsigned int plat_sd_calc_core_pos(u_register_t mpidr);
 *  With this function: CorePos = (ClusterId * 4) + CoreId
 */
func plat_sd_calc_core_pos
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc plat_sd_calc_core_pos

	/* -----------------------------------------------------
	 * unsigned long plat_get_my_entrypoint (void);
	 *
	 * Main job of this routine is to distinguish between
	 * a cold and warm boot. If the sd_sec_entry_point for
	 * this CPU is present, then it's a warm boot.
	 * -----------------------------------------------------
	 */
func plat_get_my_entrypoint
#if IMAGE_BL1
	mov	x0, #0	/*TODO*/
#else
	adr	x1, sd_sec_entry_point
	ldr	x0, [x1]
#endif
	ret
endfunc plat_get_my_entrypoint

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset.
	 * For SD case, they just wait until id matches and entry
	 * becomes non-zero.
	 * Note: cpu id check is needed here for ROM doesn't handle
	 * that yet.
	 * Clobber list : x0, x1, x2, x3
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	mov_imm	x1, AUX_BOOT_ADDR_REG
	mrs	x2, mpidr_el1
	and	x2, x2, #MPIDR_CPU_MASK
1:
	wfe
	ldr	w0, [x1]
	and	w3, w0, #AUX_BOOT_ID_MASK
	cmp	w3, w2
	b.ne	1b		/*spurious wake-up?*/
	mov_imm	x3, AUX_BOOT_ADDR_MASK
	ands	x3, x0, x3
	b.eq	1b		/*spurious wake-up?*/
	br	x3
	ret
endfunc plat_secondary_cold_boot_setup

	/* -----------------------------------------------------
	 * void platform_mem_init(void);
	 *
	 * We don't need to carry out any memory initialization
	 * on HIKEY. The Secure RAM is accessible straight away.
	 * -----------------------------------------------------
	 */
func platform_mem_init
	ret
endfunc platform_mem_init

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	x0, SD_CRASH_UART_BASE
	mov_imm	x1, SD_UART_CLK_HZ
	mov_imm	x2, SD_UART_BAUDRATE
	b	console_core_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm	x1, SD_CRASH_UART_BASE
	b	console_core_putc
endfunc plat_crash_console_putc

.macro sd_report_esr_elr_for_el id
	adr	x4, plat_err_str
	bl	asm_print_str

	adr	x4, esr_str_el\id
	bl	asm_print_str

	mrs	x4, esr_el\id
	bl	asm_print_hex

	adr	x4, elr_str_el\id
	bl	asm_print_str

	mrs	x4, elr_el\id
	bl	asm_print_hex
.endm
	/* ---------------------------------------------
	 * void plat_report_exception(unsigned int type)
	 * Function to report an unhandled exception
	 * with platform-specific means.
	 * ---------------------------------------------
	 */
func plat_report_exception
	mov	x8, x30

	mrs	x2, currentel
	lsr	x2, x2, #MODE_EL_SHIFT
	and	x2, x2, #MODE_EL_MASK
	/* Check EL1 */
	cmp	x2, #MODE_EL1
	beq	plat_report_el1
	sd_report_esr_elr_for_el 3
	b	plat_report_end
plat_report_el1:
	sd_report_esr_elr_for_el 1
plat_report_end:
	mov	x30, x8
	ret
endfunc plat_report_exception

	/* -----------------------------------------------------
	 * void plat_reset_handler(void);
	 * -----------------------------------------------------
	 */
func plat_reset_handler
	ret
endfunc plat_reset_handler

	/* -----------------------------------------------------
	 * void plat_sd_warm_entrypoint(void);
	 * warm entrypoint for SD chipsets that could be used to
	 * boot secondary cores via AUX BOOT REG.
	 * it must be aligned by AUX_BOOT_ADDR.
	 * -----------------------------------------------------
	 */
.align	AUX_BOOT_ID_BITS
func plat_sd_warm_entrypoint
	bl	plat_get_my_entrypoint
	cbz	x0, 1f
	br	x0
1:
	bl	do_panic
endfunc plat_sd_warm_entrypoint

.section .rodata.rev_err_str, "aS"
plat_err_str:
	.asciz "\nPlatform exception reporting:"
esr_str_el3:
	.asciz "\nESR_EL3: "
elr_str_el3:
	.asciz "\nELR_EL3: "
esr_str_el1:
	.asciz "\nESR_EL1: "
elr_str_el1:
	.asciz "\nELR_EL1: "

	/* --------------------------------------------------
	 * CPU Secure entry point - resume from suspend
	 *  The sd_sec_entry point is in the data section and not in .bss
	 *  even though it is zero-init. In particular, this allows
	 *  the plat_get_my_entrypoint functions to start using this variable before
	 *  the runtime memory is initialized for images which do not
	 *  need to copy the .data section from ROM to RAM.
	 * --------------------------------------------------
	 */
.section .data.sd_sec_entry_point ; .align 3
sd_sec_entry_point:
	.quad	0

	/* --------------------------------------------------
	 * NS world's cold boot entry point
	 * --------------------------------------------------
	 */
.section .data.ns_image_entrypoint ; .align 3
ns_image_entrypoint:
	.quad	0

