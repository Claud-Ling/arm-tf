#ifndef __SD_MCU_COMM_H__
#define __SD_MCU_COMM_H__

/*
 * mcomm return values
 */
#define MCOMM_OK	0
#define MCOMM_FAIL	-1
#define MCOMM_ETIME	-2	/* timeout */
#define MCOMM_EINVAL	-3	/* invalid parameter or data */
#define MCOMM_ENOMEM	-4

#ifndef __ASSEMBLY__

typedef enum _tag_mcomm_host_cmd {
	HOST_CMD_POWEROFF = 0x01,
	HOST_CMD_GET_CUR_TIME = 0x11,
	HOST_CMD_REBOOT = 0x27,
	HOST_CMD_ACK = 0x71,
	HOST_CMD_NACK = 0x72,
	HOST_CMD_RMODE_ENABLE = 0x75,
	HOST_CMD_RMODE_DISABLE = 0x77,
}mcomm_host_cmd_t;

typedef enum _tag_mcomm_mcu_cmd {
	MCU_CMD_NOTIFY_POWERMODE = 0x10,
	MCU_CMD_ACK_RMODE_ON = 0x11,
	MCU_CMD_ACK_RMODE_OFF = 0x12,
	MCU_CMD_NOTIFY_KEY = 0x51,
	MCU_CMD_ACK = 0x71,
	MCU_CMD_NACK = 0x72,
	MCU_CMD_ACK_PARA = 0x73,
}mcomm_mcu_cmd_t;

/*
 * @fn	void mcomm_init(void);
 * @brief	initial mcomm driver, only locks for now
 */
void mcomm_init(void);

/*
 * @fn int mcomm_get_mcu_cmd(uintptr_t body, size_t len, unsigned int timeout_ms);
 * @brief	try get a cmd from mcu side
 * @param[out]	ptr		- pointer of buffer to load cmd payload on success,
				  giving NULL to ignore it
 * @param[in]	len		- length of buffer pointed by body
 * @param[in]	timeout_ms	- timeout value, in unit of ms, giving -1 for infinite wait
 * @return	command code on succeed, otherwise error code
 */
int mcomm_get_mcu_cmd(uintptr_t ptr, size_t len, unsigned int timeout_ms);

/*
 * @fn int mcomm_send_mcu_cmd(uintptr_t body, size_t len, unsigned int timeout_ms);
 * @brief	send a cmd to mcu side
 * @param[in]	code		- command code
 * @param[in]	payload		- pointer to buffer of payload
 * @param[in]	len		- length of payload
 * @return	0 on succeed, otherwise error code
 */
int mcomm_send_mcu_cmd(unsigned char code, uintptr_t payload, size_t len);

#endif /*!__ASSEMBLY__*/

#endif /*__SD_MCU_COMM_H__*/
