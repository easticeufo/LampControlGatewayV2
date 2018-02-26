      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2017-2-3
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"

#define NET_LED_FILE "/sys/class/gpio/gpio68/value"
#define RUN_LED_FILE "/sys/class/leds/led-run/brightness"

static INT32 net_led_fd = -1;
static INT32 run_led_fd = -1;

static BOOL need_clear_flash = FALSE; ///< 成功清除场景绑定信息后需要闪烁

void clear_flash_led(void)
{
	need_clear_flash = TRUE;
}

static INT32 net_led_on(void)
{
	if (net_led_fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "net_led_fd < 0\n");
		return ERROR;
	}

	if (write(net_led_fd, "0", 1) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "write net_led_fd failed: %s\n", strerror(errno));
		return ERROR;
	}
	
	return OK;
}

static INT32 net_led_off(void)
{
	if (net_led_fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "net_led_fd < 0\n");
		return ERROR;
	}

	if (write(net_led_fd, "1", 1) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "write net_led_fd failed: %s\n", strerror(errno));
		return ERROR;
	}
	
	return OK;
}

static INT32 run_led_on(void)
{
	if (run_led_fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "run_led_fd < 0\n");
		return ERROR;
	}

	if (write(run_led_fd, "1", 1) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "write run_led_fd failed: %s\n", strerror(errno));
		return ERROR;
	}
	
	return OK;
}

static INT32 run_led_off(void)
{
	if (run_led_fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "run_led_fd < 0\n");
		return ERROR;
	}

	if (write(run_led_fd, "0", 1) < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "write run_led_fd failed: %s\n", strerror(errno));
		return ERROR;
	}
	
	return OK;
}

/**		  
 * @brief 网关板指示灯控制服务
 * @param no_use 未使用
 * @return 无
 */
void *led_control_daemon(void *no_use)
{
	INT32 i = 0;
	
	net_led_fd = open(NET_LED_FILE, O_RDWR);
	if (net_led_fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open failed: %s\n", strerror(errno));
		return NULL;
	}

	run_led_fd = open(RUN_LED_FILE, O_RDWR);
	if (run_led_fd < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open failed: %s\n", strerror(errno));
		SAFE_CLOSE(net_led_fd);
		return NULL;
	}

	net_led_on();

	FOREVER
	{
		if (need_clear_flash)
		{
			for (i = 0; i < 3; i++)
			{
				net_led_off();
				run_led_off();
				usleep(500 * 1000);
				net_led_on();
				run_led_on();
				usleep(500 * 1000);
			}
			need_clear_flash = FALSE;
		}
		
		run_led_off();
		usleep(500 * 1000);
		run_led_on();
		usleep(500 * 1000);
	}
}

