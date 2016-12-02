#ifndef __BOOT_LOAD_UART_H__
#define __BOOT_LOAD_UART_H__

/* uart data register */
#define UART_URBR	0	/*DLAB = 0*/
#define UART_UTBR	0	/*DLAB = 0*/

/* uart control register */
#define UART_UIER	1	/*DLAB = 0*/
#define UART_UFCR	2
#define UART_ULCR	3
#define ULCR_DLAB_BIT	7	/*bit[7]*/
#define UART_UMCR	4

#define UART_UDLL	0	/*DLAB = 1*/
#define UART_UDLM	1	/*DLAB = 1*/

/*uart status register */
#define UART_ULSR	5

#define ULSR_DR_BIT	0	/*bit[0]*/
#define ULSR_THRE_BIT	5	/*bit[5]*/

#define UART_UMSR	6
#define UART_USCR	7

#endif
