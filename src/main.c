
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author  madongfang
 * @date     2016-5-26
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include <linux/watchdog.h>
#include "base_fun.h"
#include "database.h"
#include "dev_config.h"

#define WDT_FILE "/dev/watchdog"

extern void *device_control_daemon(void *no_use);
extern void *control_socket_server(void *no_use);
extern void *serial_trans_daemon(void *no_use);

extern void *web_server(void *no_use);
extern void *upnp_fun(void *no_use);
extern void *cmd_server(void *no_use);
extern void *scene_time_trigger(void *no_use);
extern void *led_control_daemon(void *no_use);
extern void *correct_time_daemon(void *no_use);

/**
 * @brief �û����ܳ�ʼ��
 * @param[in] argc �������в����ĸ���
 * @param[in] argv �����в�������
 * @return ��
 */
void user_fun_init(INT32 argc, INT8 *argv[])
{
	/* ��������ͨ�ŷ��� */
	if(thread_create(serial_trans_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread serial_trans_daemon error\n");
		return;
	}
	
	/* �����豸���Ʒ��� */
	if(thread_create(device_control_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread device_control_daemon error\n");
		return;
	}

	/* ��̵����巢�Ϳ�������ķ��� */
	if(thread_create(control_socket_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread control_socket_server error\n");
		return;
	}

	/* ����web���� */
	if(thread_create(web_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread web_server error\n");
		return;
	}

	/* ����upnp���� */
	if(thread_create(upnp_fun, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread upnp_fun error\n");
		return;
	}

	/* ����֧���Զ���shell����ķ��� */
	if(thread_create(cmd_server, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread cmd_server error\n");
		return;
	}

	/* ����������ʱ�����ķ��� */
	if(thread_create(scene_time_trigger, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread scene_time_trigger error\n");
		return;
	}

	/* �������ذ�ָʾ�ƿ��Ʒ��� */
	if(thread_create(led_control_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread led_control_daemon error\n");
		return;
	}

	/* �����Զ�Уʱ�߳� */
	if(thread_create(correct_time_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread correct_time_daemon error\n");
		return;
	}

	return;
}

/**
* @brief ���Ź��̣߳���ʱι��
* @param no_use δʹ��
* @return ��
*/
void *watchdog_daemon(void *no_use)
{
	INT32 wdt_fd = -1;
	INT32 timeout = 30; // ��λ��

	wdt_fd = open(WDT_FILE, O_WRONLY);
	if (ERROR == wdt_fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open %s failed: %s\n", WDT_FILE, strerror(errno));
		return NULL;
	}

	ioctl(wdt_fd, WDIOC_SETTIMEOUT, &timeout);

	FOREVER
	{
		if (writen(wdt_fd, "\0", 1) != 1)
		{
			DEBUG_PRINT(DEBUG_ERROR, "feed watchdog failed\n");
		}

		sleep(timeout / 2 - 1);
	}
}

/**
 * @brief �û�ϵͳ��ʼ��
 * @param[in] argc �������в����ĸ���
 * @param[in] argv �����в�������
 * @return ��
 */
void user_system_init(INT32 argc, INT8 *argv[])
{
	/* �������Ź��߳� */
	if(thread_create(watchdog_daemon, NULL) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "create thread watchdog_daemon error\n");
		return;
	}
	
	if (database_init() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "database_init failed\n");
		return;
	}

	if (read_config_from_file() != OK)
	{
		DEBUG_PRINT(DEBUG_WARN, "read_config_from_file failed\n");
		/* ��ȡ�����ļ�ʧ��ʱ����Ĭ�����ò��� */
		set_default_config();
		if (write_config_to_file() != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "write_config_to_file failed\n");
		}
	}

	user_fun_init(argc, argv);

	FOREVER
	{
		sleep(100000);
	}
}

void print_usage(INT8 *name)
{
	printf("\nUsage:\n\n"
		"  %s [option] [parameter]\n\n"
		"  Options:\n"
		"    -debug level #set debug print level: 0-DEBUG_NONE 1-DEBUG_ERROR 2-DEBUG_WARN 3-DEBUG_NOTICE 4-DEBUG_INFO"
		"\n\n"
		, name);
	
	return;
}

/**		  
 * @brief		���������
 * @param[in] argc �������в����ĸ���
 * @param[in] argv �����в�������
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	INT32 i = 0;

	/* ����ĳЩ�źţ���ֹ���̱���Щ�ź���ֹ */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-debug") == 0)
		{
			i++;
			if (i >= argc)
			{
				print_usage(argv[0]);
				return ERROR;
			}
			set_debug_level(atoi(argv[i]));
		}
	}
	
	user_system_init(argc, argv);
	
	return OK;
}

