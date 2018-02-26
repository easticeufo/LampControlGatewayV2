      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-7-11
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "shell_cmd.h"
#include "dev_config.h"

extern INT32 websocket_get_link_num(void);
extern void print_login_info(void);

/**		  
 * @brief 自定义shell命令的服务
 * @param no_use 未使用
 * @return 无
 */
void *cmd_server(void *no_use)
{
	struct sockaddr_un sock_addr;
	INT32 sockfd = -1;
	INT8 shell_cmd[MAX_CMD_LEN] = {0};
	INT8 cmd[32] = {0};
	INT8 param1[16] = {0};
	INT8 param2[16] = {0};
	INT32 num = 0;
	INT8 str[64] = {0};
	DEVICE_CONFIG *p_dev_config = get_dev_config();
	struct tm local_time;

	sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (ERROR == sockfd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return NULL;
	}

	unlink(SHELL_CMD_SOCK_PATH);
	
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sun_family = AF_LOCAL;
	strcpy(sock_addr.sun_path, SHELL_CMD_SOCK_PATH);
	
	if (ERROR == bind(sockfd, (struct sockaddr *)&sock_addr, SUN_LEN(&sock_addr)))
	{
		DEBUG_PRINT(DEBUG_ERROR, "bind failed: %s\n", strerror(errno));
		SAFE_CLOSE(sockfd);
		return NULL;
	}

	FOREVER
	{
		memset(shell_cmd, 0, sizeof(shell_cmd));
		if (recvfrom(sockfd, shell_cmd, sizeof(shell_cmd), 0, NULL, NULL) == ERROR)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_NOTICE, "recvfrom: %s\n", strerror(errno));
				continue;
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERROR, "recvfrom failed %s\n", strerror(errno));
				sleep(2);
				continue;
			}
		}

		DEBUG_PRINT(DEBUG_NOTICE, "shell cmd: %s\n", shell_cmd);

		memset(cmd, 0, sizeof(cmd));
		memset(param1, 0, sizeof(param1));
		memset(param2, 0, sizeof(param2));
		num = sscanf(shell_cmd, "%s%s%s", cmd, param1, param2);
		
		if (strcmp(cmd, "prtHardInfo") == 0)
		{
			get_build_date(str, sizeof(str));

			printf("build time:%s\n", str);
		}
		else if (strcmp(cmd, "setDebugLevel") == 0)
		{
			if (num > 1)
			{
				set_debug_level(atoi(param1));
			}

			printf("debug level:%d\n", get_debug_level());
		}
		else if (strcmp(cmd, "prtWebsocketInfo") == 0)
		{
			printf("link number:%d\n", websocket_get_link_num());
		}
		else if (strcmp(cmd, "prtLoginInfo") == 0)
		{
			print_login_info();
		}
		else if (strcmp(cmd, "prtManufactureTime") == 0)
		{
			memset(&local_time, 0, sizeof(local_time));
			localtime_r(&p_dev_config->manufacture_time, &local_time);
			strftime(str, sizeof(str), "%F %T", &local_time);
			printf("manufacture time:%s\n", str);
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "unknow command cmd=%s\n", cmd);
		}
	}
}

