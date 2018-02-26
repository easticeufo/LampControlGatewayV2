      
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

#include "base_fun.h"
#include "serial.h"

//#define ENABLE_DEBUG // 调试宏，通过网络通信模拟串口通信

#define SERIAL_FILE "/dev/ttySP0" // uart0

#define MQ_SERIAL_REQUEST "/mq_serial_request"
#define MQ_DEVICE_CHANGE "/mq_device_change"
#define MQ_DEVICE_CONTROL "/mq_device_control"

static INT32 serial_fd = -1;

static mqd_t mq_serial_request = -1;
static mqd_t mq_device_change = -1;
static mqd_t mq_device_control = -1;

#if defined(ENABLE_DEBUG)
/**		  
 * @brief 使用网络套接字模拟串口初始化，用于测试
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 serial_init(void)
{
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	if ((serial_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(1987);
	inet_pton(AF_INET, "120.26.121.45", &server_addr.sin_addr);

	if (connect(serial_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "connect failed: %s\n", strerror(errno));
		SAFE_CLOSE(serial_fd);
		return ERROR;
	}

	return OK;
}
#else
/**		  
 * @brief 串口初始化
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 serial_init(void)
{
	struct termios options;
	
	memset(&options, 0, sizeof(struct termios));

	if (ERROR == (serial_fd = open(SERIAL_FILE, O_RDWR | O_NOCTTY)))
	{
		DEBUG_PRINT(DEBUG_ERROR, "open serial failed: %s\n", strerror(errno));
		return ERROR;
	}

	if (ERROR == tcgetattr(serial_fd, &options))
	{
		DEBUG_PRINT(DEBUG_ERROR, "tcgetattr: %s\n", strerror(errno));
		SAFE_CLOSE(serial_fd);
		return ERROR;
	}
	
	/* 设置波特率 */
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	options.c_cflag &= ~PARENB; // Parity:None
	options.c_cflag &= ~CSTOPB; // Stop bits:1
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8; // Data bits:8

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_oflag &= ~(ONLCR | OCRNL);
	options.c_iflag &= ~(INLCR | ICRNL | IGNCR);

	if (ERROR == tcsetattr(serial_fd, TCSANOW, &options))
	{
		DEBUG_PRINT(DEBUG_ERROR, "tcsetattr: %s\n", strerror(errno));
		SAFE_CLOSE(serial_fd);
		return ERROR;
	}
	
	return OK;
}
#endif

/**		  
 * @brief		 按照与stm32串口通信的协议，读取完整的一帧数据
 * @param[out] p_frame_buff 数据缓冲区
 * @param[in]  buff_len 数据缓冲区长度
 * @return	 超时或出错返回ERROR，否则返回一帧数据的总长度
 */
INT32 serial_recv_frame(UINT8 *p_frame_buff, INT32 buff_len)
{
	INT32 data_len = 0;
	
	if (NULL == p_frame_buff || buff_len < SERIAL_MIN_FRAME_LEN)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	/* 读取起始标志 */
	FOREVER
	{
		if (readn(serial_fd, &p_frame_buff[0], 1) != 1)
		{
			DEBUG_PRINT(DEBUG_INFO, "serial readn failed\n");
			return ERROR;
		}

		if (SERIAL_START_FLAG == p_frame_buff[0]) // 找到起始标志
		{
			break;
		}
		else
		{
			DEBUG_PRINT(DEBUG_WARN, "serial read start flag error, p_frame_buff[0]=%u\n", p_frame_buff[0]);
		}
	}

	/* 读取头部其他信息 */
	if (readn(serial_fd, &p_frame_buff[1], 3) != 3)
	{
		DEBUG_PRINT(DEBUG_ERROR, "serial receive frame head error\n");
		return ERROR;
	}

	/* 计算帧内数据域长度并进行校验 */
	data_len = p_frame_buff[3];
	data_len = (data_len << 8) | p_frame_buff[2];
	if ((data_len + 4 + 1) > SERIAL_MAX_FRAME_LEN || (data_len + 4 + 1) > buff_len)
	{
		DEBUG_PRINT(DEBUG_ERROR, "serial frame length error, data_len=%d\n", data_len);
		return ERROR;
	}

	/* 接收数据域+校验和 */
	if (readn(serial_fd, &p_frame_buff[4], data_len + 1) != (data_len + 1))
	{
		DEBUG_PRINT(DEBUG_ERROR, "serial receive data and checksum error\n");
		return ERROR;
	}

	/* 检查校验和 */
	if ((checksum_u8(p_frame_buff, data_len + 4) & 0xFF) != p_frame_buff[data_len + 4])
	{
		DEBUG_PRINT(DEBUG_ERROR, "serial frame checksum error\n");
		return ERROR;
	}
	
	return (data_len + 4 + 1);
}

/**		  
 * @brief		 将数据按照与stm32串口通信的协议打包成完整的一帧并发送
 * @param[in]  type 帧类型
 * @param[in]  p_data 数据域内容缓冲区，当data_len为0时，可设置为NULL
 * @param[in]  data_len 数据域长度
 * @return	 出错返回ERROR，否则返回实际发送的一帧数据的总长度
 */
INT32 serial_send_frame(UINT8 type, const UINT8 *p_data, INT32 data_len)
{
	UINT8 frame_buff[SERIAL_MAX_FRAME_LEN] = {0};
	UINT32 check_sum = 0;

	if (data_len < 0 
		|| (data_len > 0 && NULL == p_data) 
		|| (data_len + 4 + 1) > SERIAL_MAX_FRAME_LEN)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	frame_buff[0] = SERIAL_START_FLAG;
	frame_buff[1] = type;
	frame_buff[2] = data_len & 0xFF;
	frame_buff[3] = (data_len >> 8) & 0xFF;
	if (data_len > 0)
	{
		memcpy(&frame_buff[4], p_data, data_len);
	}
	check_sum = checksum_u8(frame_buff, data_len + 4);
	frame_buff[data_len + 4] = (UINT8)(check_sum & 0xFF);

	return writen(serial_fd, frame_buff, data_len + 4 + 1);
}

INT32 get_device_capa_change(INT32 *p_module_id, INT32 *p_module_type)
{
	DEVICE_MESSAGE device_msg;
	
	device_msg.capa_req.type = SERIAL_CAPA_CHG_REQ;
	if (message_queue_send_with_priority(mq_serial_request, &device_msg, sizeof(device_msg), 0) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_send failed\n");
		return ERROR;
	}
	
	if (message_queue_receive(mq_device_change, &device_msg, sizeof(device_msg)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_receive failed\n");
		return ERROR;
	}
	
	if (device_msg.type != SERIAL_CAPA_CHG_RESP)
	{
		DEBUG_PRINT(DEBUG_ERROR, "device_msg.type error(0x%02X)\n", device_msg.type);
		return ERROR;
	}
	
	*p_module_id = device_msg.capa_resp.module_id;
	*p_module_type = device_msg.capa_resp.module_type;
	return OK;
}

INT32 get_device_state_change(INT32 *p_module_id, INT32 *p_module_state)
{
	DEVICE_MESSAGE device_msg;
	
	device_msg.state_req.type = SERIAL_STATE_CHG_REQ;
	device_msg.state_req.module_id = *p_module_id;
	if (message_queue_send_with_priority(mq_serial_request, &device_msg, sizeof(device_msg), 0) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_send failed\n");
		return ERROR;
	}
	
	if (message_queue_receive(mq_device_change, &device_msg, sizeof(device_msg)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_receive failed\n");
		return ERROR;
	}
	
	if (device_msg.type != SERIAL_STATE_CHG_RESP)
	{
		DEBUG_PRINT(DEBUG_ERROR, "device_msg.type error(0x%02X)\n", device_msg.type);
		return ERROR;
	}
	
	*p_module_id = device_msg.state_resp.module_id;
	*p_module_state = device_msg.state_resp.state;
	return OK;
}


INT32 device_reset(void)
{
	DEVICE_MESSAGE device_msg;
	
	while (mq_serial_request < 0 || mq_device_change < 0 || mq_device_control < 0)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "wait message queue init\n");
		sleep(2);
	}
	
	device_msg.reset_req.type = SERIAL_DEV_RESET_REQ;
	if (message_queue_send_with_priority(mq_serial_request, &device_msg, sizeof(device_msg), 0) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_send failed\n");
		return ERROR;
	}
	
	if (message_queue_receive(mq_device_change, &device_msg, sizeof(device_msg)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_receive failed\n");
		return ERROR;
	}
	
	if (device_msg.type != SERIAL_DEV_RESET_RESP)
	{
		DEBUG_PRINT(DEBUG_ERROR, "device_msg.type error(0x%02X)\n", device_msg.type);
		return ERROR;
	}

	return OK;
}

INT32 set_device_module_state(INT32 module_id, INT32 module_state)
{
	DEVICE_MESSAGE device_msg;
	
	device_msg.ctrl_req.type = SERIAL_DEV_CTRL_REQ;
	device_msg.ctrl_req.module_id = module_id;
	device_msg.ctrl_req.state = module_state;
	if (message_queue_send_with_priority(mq_serial_request, &device_msg, sizeof(device_msg), 1) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_send failed\n");
		return ERROR;
	}
	
	if (message_queue_receive(mq_device_control, &device_msg, sizeof(device_msg)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_receive failed\n");
		return ERROR;
	}
	
	if (device_msg.type != SERIAL_DEV_CTRL_RESP)
	{
		DEBUG_PRINT(DEBUG_ERROR, "device_msg.type error(0x%02X)\n", device_msg.type);
		return ERROR;
	}
	
	return OK;
}

/**		  
 * @brief 串口通信服务,所有与串口交互的数据都由此线程进行处理,防止串口资源使用冲突
 * @param no_use 未使用
 * @return 无
 */
void *serial_trans_daemon(void *no_use)
{
	UINT8 buff[SERIAL_MAX_FRAME_LEN] = {0};
	INT32 frame_len = 0;
	DEVICE_MESSAGE device_msg;
	
	if (serial_init() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "serial_init failed\n");
		return NULL;
	}
	
	if ((mq_serial_request = message_queue_create(MQ_SERIAL_REQUEST, 10, sizeof(DEVICE_MESSAGE))) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_create MQ_SERIAL_REQUEST failed\n");
		return NULL;
	}
	
	if ((mq_device_change = message_queue_create(MQ_DEVICE_CHANGE, 10, sizeof(DEVICE_MESSAGE))) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_create MQ_DEVICE_CHANGE failed\n");
		return NULL;
	}
	if ((mq_device_control = message_queue_create(MQ_DEVICE_CONTROL, 10, sizeof(DEVICE_MESSAGE))) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "message_queue_create MQ_DEVICE_CONTROL failed\n");
		return NULL;
	}
	
	FOREVER
	{
		memset(&device_msg, 0, sizeof(device_msg));
		if (message_queue_receive(mq_serial_request, &device_msg, sizeof(device_msg)) == ERROR)
		{
			DEBUG_PRINT(DEBUG_ERROR, "mq_serial_request receive failed\n");
			sleep(1);
			continue;
		}
		
		switch (device_msg.type)
		{
			case SERIAL_CAPA_CHG_REQ:
				if (serial_send_frame(SERIAL_CAPA_CHG_REQ, NULL, 0) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
					break;
				}
				
				frame_len = serial_recv_frame(buff, sizeof(buff));
				if (frame_len < 0 || SERIAL_CAPA_CHG_RESP != buff[1])
				{
					DEBUG_PRINT(DEBUG_ERROR, "data error: frame_len=%d, cmd type=0x%02X\n"
								, frame_len, buff[1]);
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
					break;
				}
				device_msg.capa_resp.type = SERIAL_CAPA_CHG_RESP;
				device_msg.capa_resp.module_id = buff[4];
				device_msg.capa_resp.module_type = buff[5];
				message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
				break;

			case SERIAL_STATE_CHG_REQ:
				buff[0] = (UINT8)device_msg.state_req.module_id;
				if (serial_send_frame(SERIAL_STATE_CHG_REQ, buff, 1) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
					break;
				}
				
				frame_len = serial_recv_frame(buff, sizeof(buff));
				if (frame_len < 0 || SERIAL_STATE_CHG_RESP != buff[1])
				{
					DEBUG_PRINT(DEBUG_ERROR, "data error: frame_len=%d, cmd type=0x%02X\n"
								, frame_len, buff[1]);
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
					break;
				}
				device_msg.state_resp.type = SERIAL_STATE_CHG_RESP;
				device_msg.state_resp.module_id = buff[4];
				device_msg.state_resp.state = buff[6];
				device_msg.state_resp.state = (device_msg.state_resp.state << 8) | buff[5];
				message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
				break;
			case SERIAL_DEV_CTRL_REQ:
				buff[0] = (UINT8)device_msg.ctrl_req.module_id;
				buff[1] = (UINT8)(device_msg.ctrl_req.state & 0xFF);
				buff[2] = (UINT8)((device_msg.ctrl_req.state >> 8) & 0xFF);
				if (serial_send_frame(SERIAL_DEV_CTRL_REQ, buff, 3) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_control, &device_msg, sizeof(device_msg));
					break;
				}
				
				frame_len = serial_recv_frame(buff, sizeof(buff));
				if (frame_len < 0 || SERIAL_DEV_CTRL_RESP != buff[1])
				{
					DEBUG_PRINT(DEBUG_ERROR, "data error: frame_len=%d, cmd type=0x%02X\n"
								, frame_len, buff[1]);
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_control, &device_msg, sizeof(device_msg));
					break;
				}
				device_msg.ctrl_resp.type = SERIAL_DEV_CTRL_RESP;
				message_queue_send(mq_device_control, &device_msg, sizeof(device_msg));
				break;

			case SERIAL_DEV_RESET_REQ:
				if (serial_send_frame(SERIAL_DEV_RESET_REQ, NULL, 0) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
					break;
				}
				
				frame_len = serial_recv_frame(buff, sizeof(buff));
				if (frame_len < 0 || SERIAL_DEV_RESET_RESP != buff[1])
				{
					DEBUG_PRINT(DEBUG_ERROR, "data error: frame_len=%d, cmd type=0x%02X\n"
								, frame_len, buff[1]);
					device_msg.type = SERIAL_ERROR_RESP;
					message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
					break;
				}
				device_msg.reset_resp.type = SERIAL_DEV_RESET_RESP;
				message_queue_send(mq_device_change, &device_msg, sizeof(device_msg));
				break;
			default:
				DEBUG_PRINT(DEBUG_ERROR, "unknown message type, device_msg.type=0x%02x\n", device_msg.type);
				break;
		}
	}
}
