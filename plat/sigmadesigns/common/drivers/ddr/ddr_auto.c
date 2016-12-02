#include <config.h>
#include "reg_io.h"
#include "fusion_ge_memtest.h"
#include <umac.h>
#include <bug.h>
#include <types.h>


/* Serial Port define */

/* uart data register */
#define UART_UTBR	0	/*DLAB = 0*/

/* uart control register */
#define UART_UIER	1	/*DLAB = 0*/
#define UART_UFCR	2
#define UART_ULCR	3
#define UART_UMCR	4

#define UART_UDLL	0	/*DLAB = 1*/
#define UART_UDLM	1	/*DLAB = 2*/

/*uart status register */
#define UART_ULSR	5
#define UART_UMSR	6
#define UART_USCR	7

/*
 * Memory segments (32bit kernel mode addresses)
 */
#define SERIAL_BASE   0xFB005100	/* it8172 */
#define SER_CMD       5
#define SER_DATA      0x00
#define RX_READY      0x01
#define TX_BUSY       0x20

#define TIMEOUT       0xfffff	

/* Serial Porint define end */

#define __weak __attribute__((weak))
#define __text __attribute__ ((__section__(".text")))
#define __data __attribute__ ((__section__(".data")))

#define put_s serial_puts
#define MEM_MOVE_TIMES  6
#define INCR_STEP       1

#ifndef CONFIG_TR_OFS_UMAC0
 #define CONFIG_TR_OFS_UMAC0 (0x00000000)
#endif

#ifndef CONFIG_TR_OFS_UMAC1
 #define CONFIG_TR_OFS_UMAC1 (0x00000000)
#endif

#ifndef CONFIG_TR_OFS_UMAC2
 #define CONFIG_TR_OFS_UMAC2 (0x00000000)
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))
#endif

#define Sleep(x)  
#define POLY (0xffffffff)
#define PATTERN_1K 1
#define DEBUG_VAL 1

#ifdef PATTERN_1K
#define PATTERN_LEN (1024)
#endif

enum {
	RDQ = 0,
	WDQ = 1
};

struct umac_window {
	unsigned char rdq_min;
	unsigned char rdq_max;
	unsigned char rdq_target;
	unsigned char rdq_cur;
	unsigned char wdq_min;
	unsigned char wdq_max;
	unsigned char wdq_target;
	unsigned char wdq_cur;
}__attribute__((packed));
struct umac_window *window_list __data = 0;

/*
 * Parallel call flags
 */
enum {
	PCF_NORMAL = 0,  /* After task finished, need master recycle resource and can accept new task */
	PCF_AUTO_CLEANUP = 1,  /* After task finished, auto enter smp_cleaup entry */
};

#if defined(CONFIG_SW_PARALLEL_SCAN)
extern void task_entry(void);
extern int smp_assign_task_to_slave(void (*entry)(void), void *param);
extern void smp_sync_all(void);
extern void smp_sync_core(unsigned int coreid);
extern void smp_cleanup_all(void);

#define PARALLEL_CALL_WRAP(argc, flags, func, args...) do {	\
	int ret = 0;						\
	unsigned long param[ argc + 3 ] = {			\
		argc ,						\
		(unsigned long)flags,				\
		(unsigned long)func,				\
		##args						\
	};							\
	ret = smp_assign_task_to_slave(task_entry, &param[0]);	\
								\
	if(ret == -1) {						\
		put_s("All cores busy !\n");			\
		func(args);					\
	}							\
}while(0)

#define SYNC_CORE_WRAP(id) do {					\
	smp_sync_core(id);					\
}while(0)
#define SYNC_ALL_CORE_WRAP() do {				\
	smp_sync_all();						\
}while(0)
#define CLEANUP_ALL_CORE() do {					\
	smp_cleanup_all();					\
}while(0)
#else
#define PARALLEL_CALL_WRAP(argc, flags, func, args...)		\
	func(args)
#define SYNC_CORE_WRAP(id) do{}while(0)
#define SYNC_ALL_CORE_WRAP() do{}while(0)
#define CLEANUP_ALL_CORE() do{}while(0)
#endif /*CONFIG_SW_PARALLEL_SCAN*/

#ifdef CONFIG_USE_GE2D_SCAN
unsigned long __data golden_crc[4] = { 0 };
#else
unsigned long __data golden_crc[1];
#endif
static int __data pattern_len = 0;
static unsigned int umac_states = 0;
static unsigned int umac_tr_buffs[CONFIG_SIGMA_NR_UMACS];
extern unsigned char __crc_tbl_;

static const char digits[16] = "0123456789abcdef";

static void serial_puts(const char *cp)
{
	unsigned char ch;
	int i = 0;

	while (*cp) {
		do {
			/*read UART line status register*/
			ch=ReadRegByte(SERIAL_BASE+SER_CMD);
			i++;
			if (i > TIMEOUT) {
				break;
			}
		} while (0 == (ch & TX_BUSY));

		if (*cp == '\n') {
			WriteRegByte(SERIAL_BASE+SER_DATA, '\r');
			//while (0 == (ReadRegByte(SERIAL_BASE+SER_CMD) & TX_BUSY));
			WriteRegByte(SERIAL_BASE+SER_DATA, '\n');
		} else {
			WriteRegByte(SERIAL_BASE+SER_DATA, *cp);
		}
		cp++;
	}
}

static void serial_putc (const char c)
{
	unsigned char ch;

	do {
		/*read UART line status register*/
		ch = ReadRegByte(SERIAL_BASE+SER_CMD);   //MAG
	} while (0 == (ch & TX_BUSY));
	if(c=='\n') {
		WriteRegByte(SERIAL_BASE+SER_DATA, '\r');	//MAG
		//while (0 == (ReadRegByte(SERIAL_BASE+SER_CMD) & TX_BUSY));
	}
	WriteRegByte(SERIAL_BASE+SER_DATA, c);		//MAG
}

static void put8(unsigned char u)
{
	int cnt;
	unsigned ch;

	cnt=2;

	do {
		cnt--;
		ch = (unsigned char)(u>>cnt*4)&0x0F;
		serial_putc(digits[ch]);
	} while(cnt>0);

	return;
}

#if 0
static void put32(unsigned u)
{
	int cnt;
	unsigned ch;

	cnt = 8;		/* 8 nibbles in a 32 bit long */

	serial_putc('0');
	serial_putc('x');
	do {
		cnt--;
		ch = (unsigned char) (u >> cnt * 4) & 0x0F;
		serial_putc(digits[ch]);
	} while (cnt > 0);
}

static int serial_getc (void)
{
	unsigned char ch;

	do {
		/*read UART line status register*/
		ch=ReadRegByte(SERIAL_BASE+SER_CMD);  //MAG
	} while (0 == (ch & RX_READY));
	/*UART Receiver buffer register*/
	ch=ReadRegByte(SERIAL_BASE+SER_DATA);	//MAG
	return ch;
}
#endif

void * __weak  memcpy(void * dest, const void *src, unsigned int count)
{
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

/*
 *  The DDR tunning pattern
 */
/*
0xfe01fe01, 0xfe01fe01, 0xfe01fe01, 0xfe01fe01, 
0xfd02fd02, 0xfd02fd02, 0xfd02fd02, 0xfd02fd02, 
0xfb04fb04, 0xfb04fb04, 0xfb04fb04, 0xfb04fb04, 
0xf708f708, 0xf708f708, 0xf708f708, 0xf708f708, 
0xef10ef10, 0xef10ef10, 0xef10ef10, 0xef10ef10, 
0xdf20df20, 0xdf20df20, 0xdf20df20, 0xdf20df20, 
0xbf40bf40, 0xbf40bf40, 0xbf40bf40, 0xbf40bf40, 
0x7f807f80, 0x7f807f80, 0x7f807f80, 0x7f807f80, 
0x01fe01fe, 0x01fe01fe, 0x01fe01fe, 0x01fe01fe, 
0x02fd02fd, 0x02fd02fd, 0x02fd02fd, 0x02fd02fd, 
0x04fb04fb, 0x04fb04fb, 0x04fb04fb, 0x04fb04fb, 
0x08f708f7, 0x08f708f7, 0x08f708f7, 0x08f708f7, 
0x10ef10ef, 0x10ef10ef, 0x10ef10ef, 0x10ef10ef, 
0x20df20df, 0x20df20df, 0x20df20df, 0x20df20df, 
0x40bf40bf, 0x40bf40bf, 0x40bf40bf, 0x40bf40bf, 
0x807f807f, 0x807f807f, 0x807f807f, 0x807f807f
*/

static int umac_get_tr_buff(int uid, unsigned int *paddr)
{
	unsigned int base = umac_get_addr(uid);
	unsigned int tr_ofs[] = {CONFIG_TR_OFS_UMAC0,
			CONFIG_TR_OFS_UMAC1,
			CONFIG_TR_OFS_UMAC2};

	if (uid >= 0 && uid < ARRAY_SIZE(tr_ofs)) {
		*paddr = base + tr_ofs[uid];
		return SYS_NOERROR;
	} else {
		//put_s("fail to fixup tr_ofs!\n");
		return SYS_FAIL;
	}
}

static int generate_pattern(int buff)
{
	int i, j;
	unsigned char val;
	unsigned int res;
	unsigned int *lp, *hp;
	
	lp = (unsigned int *)buff;
	hp = (unsigned int *)(buff + 128);

	for (i=0; i<8; i++) {
		val = (1<<i);
		res = ((((~val)<<24) & 0xff000000) | ((val<<16) & 0xff0000) | (((~val)<<8) & 0xff00)| (val & 0xff));
		
		for (j=0; j<4; j++) {
			(*lp) = res;
			(*hp) = (~res);
			lp++;
			hp++;
		}
	}

	for (i=1; i<4; i++) {
		memcpy((unsigned int *)(buff + i*256), (void *)buff, 256);
	}

	return 0;
}

static void copy_pattern_to_mem(int dst, int src)
{
	memcpy((void*)dst, (void*)src, pattern_len);
}

#if !defined(CONFIG_USE_GE2D_SCAN)
static void init_crc_table(void)
{
	unsigned int c;
	unsigned int i, j;
	unsigned long *crc32tbl_lite = (unsigned long *)&__crc_tbl_;

	for(i = 0; i < 256; i++) {
		c = i;
		for (j=0; j < 8; j++) {
			if (c & 1)
				c = 0xedb88320L ^ (c>>1);
			else
				c = c >> 1;
		}

		crc32tbl_lite[i] = c;
	}
}

static unsigned long crc32_lite(unsigned long val, const void *ss, int len)
{

	const unsigned char *s = ss;
	unsigned long *crc32tbl_lite = (unsigned long *)&__crc_tbl_;
	while (--len >= 0)
		val = crc32tbl_lite[(val ^ *s++) & 0xff] ^ (val >> 8);
	return val;
}
#endif


#if defined(CONFIG_USE_GE2D_SCAN)
int __weak memcmp(const void * cs, const void * ct, int count)
{
        const unsigned char *su1, *su2;
        signed char res = 0;

        for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
                if ((res = *su1 - *su2) != 0)  
                        break;
        return res;
}
#endif

void * __weak memset(void * s,int c, unsigned int count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

static void reset_umac(unsigned long uid)
{
	struct umac_phy *phy = umac_get_phy(uid);

	BUG_ON(phy == NULL);
	/* Change PCTL.SCTL register to 1 */
	phy->pctl.sctl.bits.req = PCTL_CMD_CFG;
		

	/* Monitor PCTL.STAT register equal 1 */
	while (phy->pctl.stat.bits.stat != PCTL_STAT_CONFIG);

	/* Toggle and recovery PUB.PIR[6](PHYHRST) from 1->0->1 */
	phy->pub.phy_pir.bits.phyhrst ^= 1;
	phy->pub.phy_pir.bits.phyhrst ^= 1;

	/* Change PCTL.SCTL register to 2 */
	phy->pctl.sctl.bits.req = PCTL_CMD_GO;

	/* Monitor PCTL.STAT register equal 3. */
	while (phy->pctl.stat.bits.stat != PCTL_STAT_ACCESS);

}
#if defined(CONFIG_USE_GE2D_SCAN)
static int ge2d_mem_check(unsigned int src, unsigned int dest, int times, int len)
{
	unsigned long crc[4];
	
	ge_mem_move_get_crc(src, dest, len, crc);

	return (memcmp(golden_crc, crc, sizeof(crc)));
}
#else /* CONFIG_USE_GE2D_SCAN */
static int software_mem_check(unsigned int src, unsigned int dest, int times, int len)
{
	unsigned long crc;
	memcpy((void *)dest, (void *)src, len);
	crc = crc32_lite(POLY, (void *)dest, len);

	return ((crc==golden_crc[0]) ? 0 : 1);
}
#endif /* CONFIG_USE_GE2D_SCAN */

static int do_mem_test(unsigned int src, unsigned int dest, int times, int len)
{

#if defined(CONFIG_USE_GE2D_SCAN)
	return ge2d_mem_check(src, dest, times, len);
#else
	return software_mem_check(src, dest, times, len);
#endif
}

#define LCDLR1_SET_RWDELAY(lcdlr1, v, req) do{	\
	if ((req) == WDQ)			\
		(lcdlr1)->bits.wdqd = (v);	\
	else					\
		(lcdlr1)->bits.rdqsd = (v);	\
}while(0)

#define LCDLR1_GET_RWDELAY(lcdlr1, req) ({	\
	unsigned int _ret;			\
	if ((req) == WDQ)			\
		_ret = (lcdlr1)->bits.wdqd;	\
	else					\
		_ret = (lcdlr1)->bits.rdqsd;	\
	_ret;					\
})
static void set_window(unsigned int id, unsigned int port, unsigned char low, unsigned char high, int req)
{
	struct umac_phy *phy = umac_get_phy(id);
	volatile PHY_DXnLCDLR1Reg *dxnlcdlr1 = NULL;
	unsigned int cur_val;
	unsigned int target_val = (high + low)/2;
	struct umac_window *pwindow = (window_list + (id*4) + port);
	char i;

	BUG_ON(phy == NULL);
	dxnlcdlr1 = &phy->pub.phy_dx0lcdlr1 + (port * 0x10);	/* 0x40/4 */
	cur_val = LCDLR1_GET_RWDELAY(dxnlcdlr1, req);

	if ((high + low) & 0x1)
		target_val++;

	if (req == WDQ) {
		pwindow->wdq_target = target_val;
		pwindow->wdq_cur = cur_val;
	} else {
		pwindow->rdq_target = target_val;
		pwindow->rdq_cur = cur_val;
	}


	if (cur_val == 0) {
	/* Here means this register is not using, just skip */
		return;
	}

	if (cur_val == target_val) {
		return;
	}

	if (target_val > cur_val) {
		for (i=cur_val; i<=target_val; i++) {
			LCDLR1_SET_RWDELAY(dxnlcdlr1, i, req);
			reset_umac(id);
		}
	} else {
		for (i=cur_val; i >= target_val; i--) {
			LCDLR1_SET_RWDELAY(dxnlcdlr1, i, req);
			reset_umac(id);
		}
	}
}

void scan_window(unsigned int id, unsigned int port, unsigned char *pWindowLow, unsigned char *pWindowHigh, int req)
{
	struct umac_phy *phy = umac_get_phy(id);
	volatile PHY_DXnLCDLR1Reg *dxnlcdlr1 = NULL;
	unsigned int iVal;
	unsigned int start;
	int i, j;
	int t;

	BUG_ON(phy == NULL);
	dxnlcdlr1 = &phy->pub.phy_dx0lcdlr1 + (port * 0x10);	/* 0x40/4 */
	iVal = LCDLR1_GET_RWDELAY(dxnlcdlr1, req);
	start = umac_tr_buffs[id];
	for(i=iVal; i>=0; i-=INCR_STEP) {
		LCDLR1_SET_RWDELAY(dxnlcdlr1, i, req);
		reset_umac(id);
		if (do_mem_test(start, (start+pattern_len), 1, pattern_len)) {
			break;
		}

	}

	i += INCR_STEP;

	//when skip big value, the ddr may not work. so we need to change back to the origal value step by step.
	for(t=i; t<=iVal;t+=INCR_STEP) {

		LCDLR1_SET_RWDELAY(dxnlcdlr1, t, req);
		reset_umac(id);
	}

	for(j=iVal+INCR_STEP; j<=0xff; j+=INCR_STEP) {

		LCDLR1_SET_RWDELAY(dxnlcdlr1, j, req);
		reset_umac(id);
		if (do_mem_test(start, (start+pattern_len), 1, pattern_len)) {
			break;
		}

	}

	j -= INCR_STEP;

	for(t=j; t>=iVal && t >= 0; t-=INCR_STEP) {

		LCDLR1_SET_RWDELAY(dxnlcdlr1, t, req);
		reset_umac(id);
	}

	*pWindowLow = (unsigned char)i;
	*pWindowHigh = (unsigned char)j;
}

#if 0
int debug_option()
{
	char c;

	c = serial_getc();

	switch (c) {
	case 'n':
		return 0;
	case 'q':
		return -1;
	default:
		return 0;
	}
	return 0;

}
#else

#define debug_option(x) ({	\
	0;			\
}) 

#endif
static int load_pattern(void)
{
	int id, id_src = -1;
	umac_states = 0;
	for (id = 0; id < CONFIG_SIGMA_NR_UMACS; id++) {
		if (umac_is_activated(id)) {
			umac_states |= (1 << id);
			umac_get_tr_buff(id, umac_tr_buffs + id);
			if (id_src == -1) {
				id_src = id;
			}
		}
	}

	if (umac_states == 0) {
		put_s("no umac\n");
		return SYS_FAIL;
	}

	PARALLEL_CALL_WRAP(1, PCF_NORMAL, generate_pattern,
			umac_tr_buffs[id_src]);

	pattern_len = PATTERN_LEN;
#if defined (CONFIG_USE_GE2D_SCAN)
	put_s("Init GE2D\n");
	FusionGe2DInit();
	debug_option();
	SYNC_ALL_CORE_WRAP();
	/*
	 * copy pattern on selected umac.
	 * footprint in pattern_len*2 bytes memory @tmpbuf.
	 */
	ge_mem_move_get_crc(tmpbuf, (tmpbuf+pattern_len),
		pattern_len, golden_crc);
# if defined (CONFIG_SIGMA_SOC_SX7)
#  error "SX7 can't USE GE2D scan DDR, due to GE2D can't access UMAC2!!!!"
# endif
#else
	PARALLEL_CALL_WRAP(0, PCF_NORMAL, init_crc_table);
	SYNC_ALL_CORE_WRAP();

	for (id = 0; id < CONFIG_SIGMA_NR_UMACS; id++) {
		if (!(umac_states & (1 << id)) || (id == id_src))
			continue;
		PARALLEL_CALL_WRAP(2, PCF_NORMAL, copy_pattern_to_mem,
				umac_tr_buffs[id], umac_tr_buffs[id_src]);
	}

	golden_crc[0] = crc32_lite(POLY, (void *)umac_tr_buffs[id_src],
				pattern_len);
	SYNC_ALL_CORE_WRAP();
#endif
	return SYS_NOERROR;

}

static void umac_scan_set_window(unsigned int id, int wlist)
{
	int i;
	struct umac_window *pwindow = (struct umac_window *)wlist;
	struct umac_phy *phy = umac_get_phy(id);

	if (!(umac_states & (1 << id)))
		return;	/*skip inactivated umac*/

	BUG_ON(phy == NULL);
	/* Disable VT */
	phy->pub.phy_pir.bits.inhvt = 1;
	pwindow += (id*4);

	for (i=0; i<UMAC_NR_OF_LANES(id); i++) {
		scan_window(id, i, &pwindow->rdq_min, &pwindow->rdq_max, RDQ);
		scan_window(id, i, &pwindow->wdq_min, &pwindow->wdq_max, WDQ);

		set_window(id, i, pwindow->rdq_min, pwindow->rdq_max, RDQ);
		set_window(id, i, pwindow->wdq_min, pwindow->wdq_max, WDQ);
		pwindow++;
	}
	
	/* Enable VT */
	phy->pub.phy_pir.bits.inhvt = 0;
	return;
}

#define DUMP_DONE	(0xff)
static void dump_umac_window(struct umac_window *pwindow)
{
	unsigned long num[CONFIG_SIGMA_NR_UMACS] = { 0 };
	unsigned long i, j;
	struct umac_window *pw = (void *)0;
again:
	for (i=0; i<CONFIG_SIGMA_NR_UMACS; i++) {
		if (num[i] == DUMP_DONE) {
			continue;
		}

		pw = (pwindow + i*4);
		pw += num[i];

		for (j=num[i]; j<UMAC_NR_OF_LANES(i); j++) {

			num[i] = j;
			if (pw->rdq_target == 0x0 || pw->wdq_target == 0x0) {
				break;
			}
			
			/* Indicate this UMAC window already dumped */
			if (j == (UMAC_NR_OF_LANES(i) - 1)) {
				num[i] = DUMP_DONE;
			}

			put_s("UMAC");
			put8(i);
			put_s(":\n");

			put_s("\t");
			put8(pw->rdq_min);
			put_s(" ~ ");
			put8(pw->rdq_max);
			put_s(", ");

			put_s("\t    ");
			put8(pw->rdq_cur);
			put_s(",");
			put_s("\t\t    ");
			put8(pw->rdq_target);
			put_s("\n");

			put_s("\t");
			put8(pw->wdq_min);
			put_s(" ~ ");
			put8(pw->wdq_max);
			put_s(", ");

			put_s("\t    ");
			put8(pw->wdq_cur);
			put_s(",");
			put_s("\t\t    ");
			put8(pw->wdq_target);
			put_s("\n");

			pw++;
		}
	}

	for (i=0; i<CONFIG_SIGMA_NR_UMACS; i++) {
		if (umac_is_activated(i) && num[i] != DUMP_DONE)
			goto again;

	}
}

void run_ddrauto(void)
{
	int ret = 0;
	unsigned int id;
	struct umac_window window_res[12];

	window_list = &window_res[0];
 	put_s("Start\n");
	memset(window_list, 0, sizeof(window_res));
	if (load_pattern() != SYS_NOERROR)
		goto out;

	ret = debug_option();
	if (ret == -1) {
		goto out;
	}

	for (id = 0; id < CONFIG_SIGMA_NR_UMACS; id++) {
		if (umac_states & (1 << id))
			PARALLEL_CALL_WRAP(2, PCF_AUTO_CLEANUP,
			umac_scan_set_window, id, (int)window_list);
	}

#ifdef DEBUG_VAL
	put_s("done\n");
#endif
#ifdef DEBUG_VAL
//	put_s("\nStart download DDR window\n\n");
//	put_s("\t\tWindow range\tcurrent val\ttarget val\n");
#endif
	dump_umac_window(window_list);
out:
	CLEANUP_ALL_CORE();	/*important: cleanup all*/
	put_s("finish!\n");
}

