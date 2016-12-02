
#include <mmio.h>
#include <bl_common.h>
#include <delay_timer.h>
#include <debug.h>
#include <stdio.h>
#include <sd_def.h>

#define printf(...) tf_printf(__VA_ARGS__)

#define TCVR0	0x00
#define TRVR0	0x04
#define TCR0	0x08
#define TCVR1	0x0c
#define TRVR1	0x10
#define TCR1	0x14
#define TIRR	0x18
#define TIDR	0x1c

#define init_timer(id) do{				\
	mmio_write_32(SD_TIMER_BASE + TCVR##id, 0xffffffff);\
}while(0)

#define start_timer(id) do{				\
	mmio_write_32(SD_TIMER_BASE + TCR##id, 0x3);	\
}while(0)

#define stop_timer(id) do{				\
	mmio_write_32(SD_TIMER_BASE + TCR##id, 0x0);	\
}while(0)

#define read_timer(id) mmio_read_32(SD_TIMER_BASE + TRVR##id)

#define is_timer_started(id) mmio_read_32(SD_TIMER_BASE + TCR##id)

/*
 * utilize timer0
 */
static void sd_timer_start(void)
{
	if (!is_timer_started(0)) {
		init_timer(0);
		start_timer(0);
	}
}

/*
 * clock @200MHz, in descending order
 */
static uint32_t sd_timer_count(void)
{
	return read_timer(0);
}

/*
 * clock @200MHz, in ascending order
 */
static uint32_t sd_timer_count_ascend(void)
{
	return (uint32_t)(0xffffffff - sd_timer_count());
}

/*
 * clock @200MHz, in ascending order
 */
void sd_timer_show_timestamp(void)
{
	printf("%x", sd_timer_count_ascend());
	return;
}

/*
 * initialize timer0
 */
void sd_timer_init(void)
{
	static const timer_ops_t sd_timer_ops = {
		sd_timer_count,
		1,
		SD_TIMER_CLOCK_MHZ,
	};
	timer_init(&sd_timer_ops);
	sd_timer_start();
	return;
}

