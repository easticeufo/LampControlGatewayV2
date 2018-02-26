
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author  madongfang
 * @date     2016-7-6
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

INT32 test_recv_frame(INT32 sockfd, UINT8 *p_frame_buff, INT32 buff_len)
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
		if (readn(sockfd, &p_frame_buff[0], 1) != 1)
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
	if (readn(sockfd, &p_frame_buff[1], 3) != 3)
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
	if (readn(sockfd, &p_frame_buff[4], data_len + 1) != (data_len + 1))
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

INT32 test_send_frame(INT32 sockfd, UINT8 type, const UINT8 *p_data, INT32 data_len)
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

	return writen(sockfd, frame_buff, data_len + 4 + 1);
}

/**
* @brief 场景定时触发的服务
* @param no_use 未使用
* @return 无
*/
void *serial_test(void *param)
{
	INT32 sockfd = (INT64)param;
	fd_set rset;
	struct timeval timeout;
	INT32 frame_len = 0;
	UINT8 buff[SERIAL_MAX_FRAME_LEN] = {0};
		
	FOREVER
	{
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		timeout.tv_sec = 4;
		timeout.tv_usec = 0;
		
		if (select(sockfd + 1, &rset, NULL, NULL, &timeout) <= 0)
		{
			continue;
		}
		
		frame_len = test_recv_frame(sockfd, buff, sizeof(buff));
		if (frame_len < 0)
		{
			DEBUG_PRINT(DEBUG_ERROR, "data error: frame_len=%d\n", frame_len);
			SAFE_CLOSE(sockfd);
			return NULL;
		}
		
		switch (buff[1])
		{
			case SERIAL_CAPA_CHG_REQ:
				buff[0] = SERIAL_NO_CHG;
				buff[1] = 0;
				if (test_send_frame(sockfd, SERIAL_CAPA_CHG_RESP, buff, 2) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
				}
				break;
			case SERIAL_STATE_CHG_REQ:
				buff[0] = SERIAL_NO_CHG;
				buff[1] = 0;
				buff[2] = 0;
				if (test_send_frame(sockfd, SERIAL_STATE_CHG_RESP, buff, 3) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
				}
				break;
			case SERIAL_DEV_CTRL_REQ:
				if (test_send_frame(sockfd, SERIAL_DEV_CTRL_RESP, NULL, 0) == ERROR)
				{
					DEBUG_PRINT(DEBUG_ERROR, "serial_send_frame failed\n");
				}
				break;
			default:
				DEBUG_PRINT(DEBUG_ERROR, "unknown type=0x%02X\n", buff[1]);
				break;
		}
	}
}

/**		  
 * @brief		模拟串口通信程序
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	struct sockaddr_in server_addr,client_addr;
	INT32 listenfd = -1;
	INT32 connfd = -1;
	socklen_t clilen = 0;
	
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (ERROR == listenfd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(1987);
	
	if (ERROR == bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
	{
		DEBUG_PRINT(DEBUG_ERROR, "bind failed: %s\n", strerror(errno));
		SAFE_CLOSE(listenfd);
		return ERROR;
	}
	
	listen(listenfd, 5);

	FOREVER
	{
		clilen = sizeof(client_addr);
		connfd = accept(listenfd, (struct sockaddr *)&client_addr, &clilen);
		if (ERROR == connfd)
		{
			DEBUG_PRINT(DEBUG_ERROR, "accept failed: %s\n", strerror(errno));
			sleep(1);
			continue;
		}
		
		/* 启动串口通信模拟线程 */
		if(thread_create(serial_test, (void *)(INT64)connfd) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "create thread serial_test error\n");
			SAFE_CLOSE(connfd);
		}
	}
}

