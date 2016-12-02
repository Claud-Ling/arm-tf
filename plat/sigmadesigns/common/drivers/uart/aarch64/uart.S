#include <arch.h>
#include <asm_macros.S>
#include "../uart.h"

.global console_core_init
.global console_core_putc
.global console_core_getc

	/*
	 * int console_core_init(unsigned long base,
	 * unsigned int uart_clk, unsigned int baud_rate)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. This
	 * function will be accessed by console_init and
	 * crash reporting.
	 * In: x0 - console base address
	 *     w1 - Uart clock in Hz (N/A)
	 *     w2 - Baud rate (N/A, fixed @115200)
	 * Out: return 1 on success
	 * Clobber: x1
	 */
func console_core_init
	cbz	x0, console_error
	mov	w1, #0x3		/*8 bit mode, 1 stop bit*/
	strb	w1, [x0, #UART_ULCR]
	mov	w1, #0xc7		/*trigger level: 14B, RX/TX FIFO reset, enable FIFO*/
	strb	w1, [x0, #UART_UFCR]
	ldrb	w1, [x0, #UART_ULCR]	/*ULCR.DLAB=1*/
	orr	w1, w1, #(1 << ULCR_DLAB_BIT)
	strb	w1, [x0, #UART_ULCR]
	mov	w1, #1			/*baudrate 115200*/
	strb	w1, [x0, #UART_UDLL]
	mov	w1, #0
	strb	w1, [x0, #UART_UDLM]
	ldrb	w1, [x0, #UART_ULCR]	/*ULCR.DLAB=0*/
	and	w1, w1, #~(1 << ULCR_DLAB_BIT)
	strb	w1, [x0, #UART_ULCR]
	mov	w0, #1
	ret
console_error:
	mov	w0, #-1
	ret
endfunc console_core_init

	/*
	 * int console_core_putc(char ch, unsigned long base);
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In:  w0 - character to be printed
	 *	x1 - console base address
	 * Out: return -1 on error else return character
	 * Clobber: x1,x2
	 */
func console_core_putc
	/*check the input parameter*/
	cbz	x1, putc_error
	/* prepend '\r' to '\n' */
	cmp     w0, #0xA
	b.ne    2f
1:
	ldrb	w2, [x1, #UART_ULSR]
	tbz	w2, #ULSR_THRE_BIT, 1b
	mov	w2, #0xD
	strb	w2, [x1, #UART_UTBR]
2:
	ldrb	w2, [x1, #UART_ULSR]
	tbz	w2, #ULSR_THRE_BIT, 2b
	strb	w0, [x1, #UART_UTBR]
	ret
putc_error:
	mov	w0, #-1
	ret
endfunc console_core_putc

	/* ---------------------------------------------
	 * int console_core_getc(unsigned long base)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 on error.
	 * In : x0 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_getc
	/*check the input parameter*/
	cbz	x0, getc_error
1:
	ldrb	w1, [x0, #UART_ULSR]
	tbz	w1, #ULSR_DR_BIT, 1b
	ldrb	w0, [x0, #UART_URBR]
	ret
getc_error:
	mov	w0, #-1
	ret
endfunc console_core_getc
