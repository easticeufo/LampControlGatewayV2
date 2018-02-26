      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-5-30
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

#define SERIAL_MIN_FRAME_LEN 5
#define SERIAL_MAX_FRAME_LEN 16

#define SERIAL_START_FLAG 0xAA

#define SERIAL_NO_CHG 0xFF // 设备能力或状态无变化
#define SERIAL_BROADCAST 0xFF // 广播查询所有设备

/* 串口通信命令帧类型 */
#define SERIAL_CAPA_CHG_REQ 0x00
#define SERIAL_CAPA_CHG_RESP 0x01
#define SERIAL_STATE_CHG_REQ 0x02
#define SERIAL_STATE_CHG_RESP 0x03
#define SERIAL_DEV_CTRL_REQ 0x10
#define SERIAL_DEV_CTRL_RESP 0x11
#define SERIAL_DEV_RESET_REQ 0x12
#define SERIAL_DEV_RESET_RESP 0x13

/* 设备能力类型 */
#define SERIAL_4IO 0x01
#define SERIAL_8IO 0x02
#define SERIAL_12IO 0x03
#define SERIAL_4KEY 0x11
#define SERIAL_6KEY 0x12
#define SERIAL_8KEY 0x13
#define SERIAL_CLEAR_INFO 0x21 // 清空场景和绑定信息
#define SERIAL_OFF 0xFF // 设备离线

/* 设备通信消息 */
#define SERIAL_ERROR_RESP 0xFF

typedef struct
{
	INT32 type;
}MSG_CAPA_REQ;

typedef struct
{
	INT32 type;
	INT32 module_id;
	INT32 module_type;
}MSG_CAPA_RESP;

typedef struct
{
	INT32 type;
	INT32 module_id;
}MSG_STATE_REQ;

typedef struct
{
	INT32 type;
	INT32 module_id;
	INT32 state;
}MSG_STATE_RESP;

typedef struct
{
	INT32 type;
	INT32 module_id;
	INT32 state;
}MSG_CTRL_REQ;

typedef struct
{
	INT32 type;
}MSG_CTRL_RESP;

typedef struct
{
	INT32 type;
}MSG_RESET_REQ;

typedef struct
{
	INT32 type;
}MSG_RESET_RESP;

/**		  
 * @brief 控制设备的通信的消息结构
 */
typedef union
{
	INT32 type;
	MSG_CAPA_REQ capa_req;
	MSG_CAPA_RESP capa_resp;
	MSG_STATE_REQ state_req;
	MSG_STATE_RESP state_resp;
	MSG_CTRL_REQ ctrl_req;
	MSG_CTRL_RESP ctrl_resp;
	MSG_RESET_REQ reset_req;
	MSG_RESET_RESP reset_resp;
}DEVICE_MESSAGE;

INT32 serial_init(void);
INT32 serial_recv_frame(UINT8 *p_frame_buff, INT32 buff_len);
INT32 serial_send_frame(UINT8 type, const UINT8 *p_data, INT32 data_len);
INT32 get_device_capa_change(INT32 *p_module_id, INT32 *p_module_type);
INT32 get_device_state_change(INT32 *p_module_id, INT32 *p_module_state);
INT32 set_device_module_state(INT32 module_id, INT32 module_state);
INT32 device_reset(void);

#endif

