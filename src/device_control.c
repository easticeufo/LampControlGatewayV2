      
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
#include "database.h"

#define CTRL_SOCK_PATH "/tmp/ctrl_sock"
#define LAMP_MODULE_STATE_CHG 1

/**		  
 * @brief 控制设备的unix套接字传递的数据结构
 */
typedef struct
{
	INT32 type;
	UINT8 info[10];
}CTRL_SOCK_DATA;

extern void websocket_statechange_send_msg(const INT8 *p_msg);
extern void clear_flash_led(void);

static INT32 ctrl_client_sock = -1; // 灯控制发送套接字
static INT32 ctrl_recv_sock = -1; // 灯控制接收套接字

INT32 ctrl_sock_init(void)
{
	ctrl_client_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (ERROR == ctrl_client_sock)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	return OK;
}

INT32 set_lamp_module_state(UINT8 lamp_module_id, UINT16 state)
{
	struct sockaddr_un ctrl_sock_addr;
	CTRL_SOCK_DATA ctrl_sock_data;

	DEBUG_PRINT(DEBUG_NOTICE, "set lamp state module_id=0x%02X,state=0x%04X\n"
				, lamp_module_id, state);
	memset(&ctrl_sock_addr, 0, sizeof(ctrl_sock_addr));
	ctrl_sock_addr.sun_family = AF_LOCAL;
	strcpy(ctrl_sock_addr.sun_path, CTRL_SOCK_PATH);

	memset(&ctrl_sock_data, 0, sizeof(ctrl_sock_data));
	ctrl_sock_data.type = LAMP_MODULE_STATE_CHG;
	ctrl_sock_data.info[0] = lamp_module_id;
	ctrl_sock_data.info[1] = (UINT8)(state & 0xFF);
	ctrl_sock_data.info[2] = (UINT8)((state >> 8) & 0xFF);
	if (sendto(ctrl_client_sock, &ctrl_sock_data, sizeof(ctrl_sock_data), 0
			   , (struct sockaddr *)&ctrl_sock_addr, sizeof(ctrl_sock_addr)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sendto failed: %s\n", strerror(errno));
		return ERROR;
	}

	return OK;
}

INT32 device_init(void)
{
	LAMP_MODULE lamp_module[MAX_LAMP_MODULE];
	INT32 module_num = 0;
	INT32 i = 0;
	INT32 module_id = 0;
	INT32 module_type = 0;
	INT32 number = 0;
	INT32 module_state = 0;
	
	module_num = get_lamp_module(lamp_module, MAX_LAMP_MODULE);
	
	if (device_reset() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "device_reset failed\n");
		return ERROR;
	}

	if (clear_device_table() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "clear_device_table failed\n");
		return ERROR;
	}

	/* 设备能力变化查询 */
	FOREVER
	{
		if (get_device_capa_change(&module_id, &module_type) == ERROR)
		{
			DEBUG_PRINT(DEBUG_ERROR, "get_device_capa_change failed\n");
			break;
		}
		if (SERIAL_NO_CHG == module_id) // 所有设备能力都无变化
		{
			break;
		}

		switch (module_type)
		{
			case SERIAL_4IO:
			case SERIAL_8IO:
			case SERIAL_12IO:
				if (SERIAL_4IO == module_type)
				{
					number = 4;
				}
				else if (SERIAL_8IO == module_type)
				{
					number = 8;
				}
				else if (SERIAL_12IO == module_type)
				{
					number = 12;
				}
				else
				{
					number = 0;
				}

				for (module_state = -1, i = 0; i < module_num; i++)
				{
					if (lamp_module[i].id == module_id)
					{
						module_state = lamp_module[i].state;
						break;
					}
				}

				if (-1 == module_state) // 新增设备需要获取设备状态
				{
					if (get_device_state_change(&module_id, &module_state) == ERROR)
					{
						DEBUG_PRINT(DEBUG_ERROR, "get_device_state_change failed\n");
						module_state = 0;
					}
				}
				else // 已有设备需要设置原来存储的状态
				{
					if (set_device_module_state(module_id, module_state) != OK)
					{
						DEBUG_PRINT(DEBUG_ERROR, "set_device_module_state failed\n");
					}
				}

				if (add_lamp_module(module_id, number, module_state) != OK)
				{
					DEBUG_PRINT(DEBUG_ERROR, "add_lamp_module failed\n");
				}
				
				break;

			case SERIAL_4KEY:
			case SERIAL_6KEY:
			case SERIAL_8KEY:
				if (SERIAL_4KEY == module_type)
				{
					number = 4;
				}
				else if (SERIAL_6KEY == module_type)
				{
					number = 6;
				}
				else if (SERIAL_8KEY == module_type)
				{
					number = 8;
				}
				else
				{
					number = 0;
				}

				/* 更新数据库 */
				if (add_key_module(module_id, number) != OK)
				{
					DEBUG_PRINT(DEBUG_ERROR, "add_key_module failed\n");
				}
				
				break;

			default:
				DEBUG_PRINT(DEBUG_ERROR, "capa_type error! value=0x%02X\n", module_type);
				break;
		}
	}

	websocket_statechange_send_msg("light");

	return OK;
}

/**		  
 * @brief 设备控制服务
 * @param no_use 未使用
 * @return 无
 */
void *device_control_daemon(void *no_use)
{
	INT32 module_id = 0;
	INT32 module_type = 0;
	INT32 module_state = 0;
	INT32 number = 0;

	if (device_init() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "device_init failed\n");
	}
	
	FOREVER
	{
		usleep(200 * 1000);
		
		/* 设备能力变化查询 */
		FOREVER
		{
			if (get_device_capa_change(&module_id, &module_type) == ERROR)
			{
				DEBUG_PRINT(DEBUG_ERROR, "get_device_capa_change failed\n");
				break;
			}
			if (SERIAL_NO_CHG == module_id) // 所有设备能力都无变化
			{
				break;
			}
			
			switch (module_type)
			{
				case SERIAL_4IO:
				case SERIAL_8IO:
				case SERIAL_12IO:
					if (SERIAL_4IO == module_type)
					{
						number = 4;
					}
					else if (SERIAL_8IO == module_type)
					{
						number = 8;
					}
					else if (SERIAL_12IO == module_type)
					{
						number = 12;
					}
					else
					{
						number = 0;
					}

					/* 能力变化后，IO模块的状态必然发生变化，获取变化后的状态 */
					if (get_device_state_change(&module_id, &module_state) == ERROR)
					{
						DEBUG_PRINT(DEBUG_ERROR, "get_device_state_change failed\n");
						break;
					}
					/* 更新数据库 */
					if (change_device_to_lamp(module_id, number, module_state) != OK)
					{
						DEBUG_PRINT(DEBUG_ERROR, "change_device_to_lamp failed\n");
					}
					
					break;

				case SERIAL_4KEY:
				case SERIAL_6KEY:
				case SERIAL_8KEY:
					if (SERIAL_4KEY == module_type)
					{
						number = 4;
					}
					else if (SERIAL_6KEY == module_type)
					{
						number = 6;
					}
					else if (SERIAL_8KEY == module_type)
					{
						number = 8;
					}
					else
					{
						number = 0;
					}

					/* 更新数据库 */
					if (change_device_to_key(module_id, number) != OK)
					{
						DEBUG_PRINT(DEBUG_ERROR, "change_device_to_key failed\n");
					}
					
					break;
					
				case SERIAL_OFF:
					/* 更新数据库 */
					if (delete_device(module_id) != OK)
					{
						DEBUG_PRINT(DEBUG_ERROR, "delete_device failed\n");
					}
					break;

				case SERIAL_CLEAR_INFO:
					/* 清空场景 绑定信息 */
					if (clear_bind_and_scene() != OK)
					{
						DEBUG_PRINT(DEBUG_ERROR, "clear_bind_and_scene failed\n");
					}
					else
					{
						clear_flash_led();
					}
					break;
				default:
					DEBUG_PRINT(DEBUG_ERROR, "capa_type error! value=0x%02X\n", module_type);
					break;
			}

			websocket_statechange_send_msg("light");
		}

		/* 设备状态变化查询 */
		FOREVER
		{
			module_id = SERIAL_BROADCAST;
			if (get_device_state_change(&module_id, &module_state) == ERROR)
			{
				DEBUG_PRINT(DEBUG_ERROR, "get_device_state_change failed\n");
				break;
			}
			
			if (SERIAL_NO_CHG == module_id) // 所有设备状态都无变化
			{
				break;
			}

			/* 更新数据库 */
			if (change_device_state(module_id, module_state) != OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "change_device_state failed\n");
			}
		}
	}
	
	return NULL;
}

/**		  
 * @brief 向继电器板发送开关命令的服务
 * @param no_use 未使用
 * @return 无
 */
void *control_socket_server(void *no_use)
{
	struct sockaddr_un ctrl_sock_addr;
	fd_set rset;
	struct timeval timeout;
	INT32 ret = 0;
	CTRL_SOCK_DATA ctrl_sock_data;
	UINT8 module_id = 0;
	INT32 module_state = 0;

	ctrl_recv_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (ERROR == ctrl_recv_sock)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return NULL;
	}

	unlink(CTRL_SOCK_PATH);
	
	memset(&ctrl_sock_addr, 0, sizeof(ctrl_sock_addr));
	ctrl_sock_addr.sun_family = AF_LOCAL;
	strcpy(ctrl_sock_addr.sun_path, CTRL_SOCK_PATH);
	
	if (ERROR == bind(ctrl_recv_sock, (struct sockaddr *)&ctrl_sock_addr, SUN_LEN(&ctrl_sock_addr)))
	{
		DEBUG_PRINT(DEBUG_ERROR, "bind failed: %s\n", strerror(errno));
		SAFE_CLOSE(ctrl_recv_sock);
		return NULL;
	}
	
	/* 设备控制命令处理 */
	FOREVER
	{
		FD_ZERO(&rset);
		FD_SET(ctrl_recv_sock, &rset);
		timeout.tv_sec = 4;
		timeout.tv_usec = 0;
		ret = select(ctrl_recv_sock + 1, &rset, NULL, NULL, &timeout);
		if (ret < 0)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_NOTICE, "select: %s\n", strerror(errno));
				continue;
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERROR, "select failed: %s\n", strerror(errno));
				sleep(4);
				continue;
			}
		}
		else if (0 == ret)
		{
			DEBUG_PRINT(DEBUG_INFO, "select timeout\n");
		}
		else
		{
			memset(&ctrl_sock_data, 0, sizeof(ctrl_sock_data));
			if (recvfrom(ctrl_recv_sock, &ctrl_sock_data, sizeof(ctrl_sock_data), 0, NULL, NULL) == ERROR)
			{
				if (EINTR == errno)
				{
					DEBUG_PRINT(DEBUG_NOTICE, "recvfrom: %s\n", strerror(errno));
					continue;
				}
				else
				{
					DEBUG_PRINT(DEBUG_ERROR, "recvfrom failed %s\n", strerror(errno));
					sleep(4);
					continue;
				}
			}
			
			if (LAMP_MODULE_STATE_CHG == ctrl_sock_data.type)
			{
				module_id = ctrl_sock_data.info[0];
				module_state = ctrl_sock_data.info[2];
				module_state = (module_state << 8) | ctrl_sock_data.info[1];
				if (set_device_module_state(module_id, module_state) == OK)
				{
					/* 更新数据库中的灯状态 */
					ret = set_lamp_module_state_to_db(module_id, module_state);
					if (ERROR == ret)
					{
						DEBUG_PRINT(DEBUG_ERROR, "set_lamp_module_state_to_db failed\n");
						continue;
					}
					websocket_statechange_send_msg("light");
				}
				else
				{
					DEBUG_PRINT(DEBUG_ERROR, "set_device_module_state failed\n");
				}
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERROR, "wrong control type type=%d\n", ctrl_sock_data.type);
			}
		}
	}
}

