
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author  madongfang
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

/**		  
 * @brief		自定义shell命令主程序
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	INT8 shell_cmd[MAX_CMD_LEN] = {0};
	INT32 i = 0;
	INT32 len = 0;
	INT32 sockfd = 0;
	struct sockaddr_un sock_addr;

	memset(&sock_addr, 0, sizeof(sock_addr));

	for (i = 0; i < argc; i++)
	{
		len += snprintf(shell_cmd + len, MAX_CMD_LEN - len, "%s ", argv[i]);
		if (len >= MAX_CMD_LEN)
		{
			DEBUG_PRINT(DEBUG_ERROR, "command exceed max length\n");
			break;
		}
	}

	sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (ERROR == sockfd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "socket failed: %s\n", strerror(errno));
		return ERROR;
	}

	sock_addr.sun_family = AF_LOCAL;
	strcpy(sock_addr.sun_path, SHELL_CMD_SOCK_PATH);

	if (sendto(sockfd, shell_cmd, MAX_CMD_LEN, 0
		, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sendto failed: %s\n", strerror(errno));
		SAFE_CLOSE(sockfd);
		return ERROR;
	}

	SAFE_CLOSE(sockfd);
	return OK;
}

