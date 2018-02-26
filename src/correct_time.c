      
/**@file �Զ�Уʱ
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2017-12-26
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"

/**
* @brief ������ʱ�����ķ���
* @param no_use δʹ��
* @return ��
*/
void *correct_time_daemon(void *no_use)
{
	time_t last_time = 0;
	time_t now_time = 0;
	INT32 ret = 0;
	
	FOREVER
	{
		now_time = time(NULL);
		if (now_time - last_time > 60 * 60) // ÿ��1Сʱͬ��һ��
		{
			ret = system("hwclock -s --utc"); // ��Ӳ��ʱ��ͬ����ϵͳʱ��
			DEBUG_PRINT(DEBUG_NOTICE, "system(hwclock -s --utc) ret=%d\n", ret);
			last_time = now_time;
		}
		
		sleep(60);
	}
}

