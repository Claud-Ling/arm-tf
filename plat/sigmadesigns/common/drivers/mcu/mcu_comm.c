/*
 * A generic bidirection mcu_comm driver is implemented here, which works in
 * polling mode and thus suits for scenarios when IRQ is either not enabled 
 * or disabled, i.e.
 * early boot phase, or underlying pm driver.
 *
 * TODO: might need to add support for IRQ mode.
 *
 * Note that it requires an mcu_comm 1.0.3 compatible mcu binary for the whole
 * ideas here are built based on mcu_comm extension.
 *
 * Author: Tony He
 * Date:   2016/12/13
 *
 */

#include <arch_helpers.h>
#include <bl_common.h>
#include <mmio.h>
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <delay_timer.h>
#include <platform_def.h>
#include <sd_private.h>
#include <mcu/mcomm_ctrl0.h>
#include <mcu/mcomm_ctrl1.h>
#include <sd_mcu_comm.h>
#include "mcu_reg.h"

/*
 * Locks regardings, make use of bakery lock.
 * Note that can't use 'exclusive' based spinlocks here for psci might invoke
 * mcomm methods before enable dcache in early warmboot path.
 */
#include <bakery_lock.h>
#define MCOMM_DEFINE_LOCK(name) DEFINE_BAKERY_LOCK(name)
#define MCOMM_LOCK_INIT(l) bakery_lock_init(l)
#define MCOMM_LOCK_GET(l) bakery_lock_get(l)
#define MCOMM_LOCK_RELEASE(l) bakery_lock_release(l)

#define CTRLSEG_LEN			2
#define DATASEG_LEN			(16 - CTRLSEG_LEN)

#define MIPS_CTRLSEG_START		((uintptr_t)&STB_REG(hcmd)[0])
#define MIPS_DATASEG_START		(MIPS_CTRLSEG_START + CTRLSEG_LEN)

#define MCU_CTRLSEG_START		((uintptr_t)&STB_REG(mcmd)[0])
#define MCU_DATASEG_START		(MCU_CTRLSEG_START + CTRLSEG_LEN)

#define MAX_PACKAGE_COUNT		(1<<4)	// 4 bit
#define MCU_COMM_BUFCOUNT_MAX		210

#define MK_MCOMM_CTRL1(f,r,c,err,len)	(((f) << MCOMM_CTRL1_f_SHIFT) |	\
					((r) << MCOMM_CTRL1_r_SHIFT) |	\
					((c) << MCOMM_CTRL1_c_SHIFT) |	\
					((err) << MCOMM_CTRL1_err_SHIFT) |	\
					((len) & ((1 << MCOMM_CTRL1_len_WIDTH) - 1)))

/*
 * mcomm data packet format:
 *
 * +-----+------+---------------+----------+-----+
 * | STB | Code |    Payload    | Checksum | ETB |
 * +-----+------+---------------+----------+-----+->
 * | 0xFF|1 byte| 4|12|28 bytes | 1 byte   |0xFE |
 * +-----+------+---------------+----------+-----+
 *      /        \
 *     /          \
 *    +----+-------+
 *    |type|  code |
 *    +----+-------+
 *       7  6     0
 *
 */

#define FRAME_STB			0xFF
#define FRAME_ETB			0xFE
#define FRAME_TYPE_MASK			(1 << 7)
#define FRAME_CODE_MASK			0x7F

#define FRAME_TYPE(msg)			((msg) & FRAME_TYPE_MASK)
#define FRAME_CODE(msg)			((msg) & FRAME_CODE_MASK)

/*message type*/
#define FRAME_TYPE_HOST			0
#define FRAME_TYPE_MCU			(1 << 7)


typedef struct _tag_mcu_comm_param{
	unsigned int buf_len;
	unsigned char buffer[MCU_COMM_BUFCOUNT_MAX];
}mcu_comm_param_t;

#define MCOMM_TIMEOUT_MS	1000	/*timout 1s*/
static struct mcu_comm_driver {
#define MCOMM_RETRY_NUM		5
	unsigned retry;
	unsigned last_cmd;
#define MCOMM_STATE_NONE	(-1)
#define MCOMM_STATE_OK		0	/*succeed*/
#define MCOMM_STATE_TIMEOUT	1	/*timeout*/
#define MCOMM_STATE_INT		2	/*interrupted*/
	unsigned last_state;
	int users;
} mcomm_drv, *pmcomm_drv = &mcomm_drv;

/*
 * Define locks
 *
 * mcomm_refile_lock is used to protect register files to avoid race condition in SMP case;
 *
 * mcomm_mailbox_lock is used to protect mailbox register. The point is mailbox will be modified
 * by the top half, so lock here shall put that in mind and be safe to use in atomic context.
 *
 */
static MCOMM_DEFINE_LOCK(mcomm_regfile_lock);
static MCOMM_DEFINE_LOCK(mcomm_mailbox_lock);

/*
 * check if new mcu request is available by test E1.C bit.
 * This's in accordance with 1.0.3 mcu_comm protocol.
 */
static int check_mcu_request(void* arg)
{
	volatile union MCOMM_CTRL1Reg *m1, *e1;
	m1 = (volatile union MCOMM_CTRL1Reg*) (MIPS_CTRLSEG_START + 1);
	e1 = (volatile union MCOMM_CTRL1Reg*) (MCU_CTRLSEG_START + 1);
	if (e1->bits.f)
		return (e1->bits.c != m1->bits.r);
	else
		return 0;
}

/*
 * check if mcu response is available by test E1.R bit.
 * This's in accordance with 1.0.3 mcu_comm protocol.
 */
static int check_mcu_resp(void* arg)
{
	volatile union MCOMM_CTRL1Reg *m1, *e1;
	m1 = (volatile union MCOMM_CTRL1Reg*) (MIPS_CTRLSEG_START + 1);
	e1 = (volatile union MCOMM_CTRL1Reg*) (MCU_CTRLSEG_START + 1);
	return (e1->bits.r == m1->bits.c);
}

/*
 * return value:
 * 	0	- timeout
 *	>0	- completion ok
 *	<0	- error code
 */
int wait_for_completion_timeout(int(*complete)(void*), void*arg, unsigned int timeout_ms)
{
	if (timeout_ms == -1) {
		while(!complete(arg));
		return 1;
	} else if (timeout_ms == 0) {
		return complete(arg);
	} else {
		int try = 0;
		while(!complete(arg)) {
			if (timeout_ms == 0) {
				break;
			}

			udelay(4);
			try++;
			if (!(try & 0xFF))
				timeout_ms--;
		}

		return timeout_ms;
	}
}

static int mcomm_drv_get(struct mcu_comm_driver *drv)
{
	MCOMM_LOCK_GET(&mcomm_regfile_lock);
	drv->users++;
	return 0;
}

static int mcomm_drv_put(struct mcu_comm_driver *drv)
{
	MCOMM_LOCK_RELEASE(&mcomm_regfile_lock);
	drv->users--;
	return 0;
}

static void mips_request_mcu(unsigned len)
{
	unsigned char temp;
	volatile union MCOMM_CTRL1Reg *m1, *e1;

	MCOMM_LOCK_GET(&mcomm_mailbox_lock);
	/* set ctrl1, M1.C = ~E1.R; */
	m1 = (volatile union MCOMM_CTRL1Reg*) (MIPS_CTRLSEG_START + 1);
	e1 = (volatile union MCOMM_CTRL1Reg*) (MCU_CTRLSEG_START + 1);
	temp = MK_MCOMM_CTRL1(1, m1->bits.r, !e1->bits.c, 0, len);
	m1->val = temp;	/*set all bits in one write*/
	dmbsy();

	STB_REG(mips00).bits.mips_intr_mcu ^= 1; /*reverse*/
	MCOMM_LOCK_RELEASE(&mcomm_mailbox_lock);
}

static void mips_response_mcu(void)
{
	volatile union MCOMM_CTRL1Reg *m1, *e1;

	MCOMM_LOCK_GET(&mcomm_mailbox_lock);
	/* set ctrl1, M1.R = E1.C; */
	m1 = (volatile union MCOMM_CTRL1Reg*)(MIPS_CTRLSEG_START + 1);
	e1 = (volatile union MCOMM_CTRL1Reg*)(MCU_CTRLSEG_START + 1);
	if (e1->bits.f) {
		m1->bits.f = 1;
		m1->bits.r = e1->bits.c;
		dmbsy();
	}

	STB_REG(mips00).bits.mips_res_mcu ^= 1; /*reverse*/
	MCOMM_LOCK_RELEASE(&mcomm_mailbox_lock);
}

/*
 * return value
 * 	0	- ok
 * 	-EINVAL	- invalid parameter
 * 	-ETIME	- timeout
 * 	-EINTR	- interrupted by signal
 */
static int send_one_frame(mcu_comm_param_t * param)
{
	int ret;
	unsigned char total, stamp, len, temp;
	unsigned int i;

	/*init value*/
	total = (param->buf_len + DATASEG_LEN - 1) / DATASEG_LEN;
	if ( total > MAX_PACKAGE_COUNT-1 ) {
		ERROR("frame data (%d) exceeds %d bytes!\n", param->buf_len, MCU_COMM_BUFCOUNT_MAX);
		return MCOMM_EINVAL;
	}

	stamp = 1;

	assert(pmcomm_drv != NULL);
	mcomm_drv_get(pmcomm_drv);

	/*collect debug information*/
	pmcomm_drv->retry = MCOMM_RETRY_NUM;
	pmcomm_drv->last_cmd = param->buffer[1];	/*cmd id*/
	pmcomm_drv->last_state = MCOMM_STATE_NONE;
	INFO("sending mcu cmd %x len %d\n", pmcomm_drv->last_cmd, param->buf_len);
	while( stamp <= total )
	{
		//fill in Control Segment
		//except for mcomm_ctrl1
		temp = (total<<4) + stamp;

		mmio_write_8(MIPS_CTRLSEG_START, temp);

		len = SD_MIN((int)(param->buf_len - (stamp-1)*DATASEG_LEN), DATASEG_LEN);

		//fill in Data Segment
		for( i=0; i < len; i++ )
		{
			temp = param->buffer[(stamp-1)*DATASEG_LEN + i];
			mmio_write_8(MIPS_DATASEG_START+i, temp);
		}

		mips_request_mcu(len);

		/*wait for mcu acknowledge*/
		ret = wait_for_completion_timeout(check_mcu_resp, NULL, MCOMM_TIMEOUT_MS);
		if (ret == 0) {
			//timeout
			if (pmcomm_drv->retry-- > 0) {
				WARN("sending mcu cmd %x timeout -%d-\n", pmcomm_drv->last_cmd, pmcomm_drv->retry);
				continue;
			} else {
				pmcomm_drv->last_state = MCOMM_STATE_TIMEOUT;
				ret = MCOMM_ETIME;
				break;
			}
		}

		stamp ++;
	}

	if (stamp > total) {
		pmcomm_drv->last_state = MCOMM_STATE_OK;
		ret = 0;
	}

	INFO("finish mcu cmd %x, ret %d\n", pmcomm_drv->last_cmd, ret);
	mcomm_drv_put(pmcomm_drv);
	return ret;
}

/*
 * @fn int mcomm_get_mcu_cmd(uintptr_t body, size_t len, unsigned int timeout_ms);
 * @brief	try get a cmd from mcu side
 * @param[out]	ptr		- pointer of buffer to load cmd payload on success,
				  giving NULL to ignore it
 * @param[in]	len		- length of buffer pointed by body
 * @param[in]	timeout_ms	- timeout value, in unit of ms, giving -1 for infinite wait
 * @return	command code on succeed, otherwise error code
 */
int mcomm_get_mcu_cmd(uintptr_t ptr, size_t len, unsigned int timeout_ms)
{
	int i;
	unsigned char sum = 0;
	mcu_comm_param_t params;
	volatile union MCOMM_CTRL0Reg *e0;
	volatile union MCOMM_CTRL1Reg *e1;
	e0 = (volatile union MCOMM_CTRL0Reg*)MCU_CTRLSEG_START;
	e1 = (volatile union MCOMM_CTRL1Reg*)(MCU_CTRLSEG_START + 1);
	do {
		if (0 == wait_for_completion_timeout(check_mcu_request, NULL, timeout_ms)) {
			WARN("%s: timeout\n", __func__);
			return MCOMM_ETIME;
		}
		INFO("==> total:%d, stamp:%d, len:%d\n",
			e0->bits.total, e0->bits.stamp, e1->bits.len);
		for (i = 0; i < e1->bits.len; i++) {
			params.buffer[params.buf_len + i] = mmio_read_8(MCU_DATASEG_START + i);
		}
		params.buf_len += e1->bits.len;
		mips_response_mcu();	/*response remote*/
	} while(e0->bits.stamp < e0->bits.total);

	/*
	 * validate data packet
	 */
	if (FRAME_STB == params.buffer[0] &&
	    FRAME_TYPE_MCU == FRAME_TYPE(params.buffer[1]) &&
	    FRAME_ETB == params.buffer[params.buf_len - 1]) {
		for (i = 1; i < params.buf_len - 1; i++)
			sum += params.buffer[i];

		if (sum == 0) {
			INFO("%s: code %x payload %d\n", __func__,
				FRAME_CODE(params.buffer[1]), params.buf_len - 4);
			/*copy payload if needed*/
			if (ptr && len >= params.buf_len - 4)
				memcpy((void*)ptr, params.buffer+2, params.buf_len-4);
			/*return message code*/
			return FRAME_CODE(params.buffer[1]);
		}
	}

	WARN("%s: error data packet: %x %x...\n", __func__, params.buffer[0], params.buffer[1]);
	return MCOMM_EINVAL;
}

/*
 * @fn int mcomm_send_mcu_cmd(uintptr_t body, size_t len, unsigned int timeout_ms);
 * @brief	send a cmd to mcu side
 * @param[in]	code		- command code
 * @param[in]	payload		- pointer to buffer of payload
 * @param[in]	len		- length of payload
 * @return	0 on succeed, otherwise error code
 */
int mcomm_send_mcu_cmd(unsigned char code, uintptr_t payload, size_t len)
{
	int i;
	unsigned char sum = 0;
	mcu_comm_param_t params;
	assert(len <= MCU_COMM_BUFCOUNT_MAX - 4);
	assert(len == 4 || len == 12 || len == 28);
	/* packetize data */
	params.buf_len = len + 4;	/*length*/
	params.buffer[0] = FRAME_STB;	/*STB*/
	params.buffer[1] = (code & FRAME_CODE_MASK);	/*Message*/
	memcpy(&params.buffer[2], (void*)payload, len);	/*Payload*/
	for (i = 0; i < len + 1; i++)
		sum += params.buffer[1 + i];
	params.buffer[len + 2] = (char)0 - sum;		/*Checksum*/
	params.buffer[len + 3] = FRAME_ETB;		/*ETB*/
	INFO("%s: code %x, payload %ld\n", __func__, code, len);
	return send_one_frame(&params);
}

/*
 * initial mcomm driver, only locks for now
 */
void mcomm_init(void)
{
	MCOMM_LOCK_INIT(&mcomm_regfile_lock);
	MCOMM_LOCK_INIT(&mcomm_mailbox_lock);
}
