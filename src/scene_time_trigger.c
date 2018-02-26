      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-9-28
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "database.h"

void get_trigger_wday(const INT8 *p_wday_str, BOOL *wday)
{
	const INT8 *ptr = NULL;
	const INT8 *comma_ptr = NULL;
	INT8 str[16] = {0};
	INT32 week_day = -1;
	INT32 i = 0;
	
	for (i = 0; i < 7; i++)
	{
		wday[i] = FALSE;
	}
	ptr = p_wday_str;
	while (strlen(ptr) != 0)
	{
		memset(str, 0, sizeof(str));
		comma_ptr = strchr(ptr, ',');
		if (comma_ptr != NULL)
		{
			memcpy(str, ptr, comma_ptr - ptr);
			ptr = comma_ptr + 1;
		}
		else
		{
			strcpy(str, ptr);
			ptr = ptr + strlen(ptr);
		}
		week_day = atoi(str);
		if (week_day >= 0 && week_day <= 6)
		{
			wday[week_day] = TRUE;
		}
	}
	return;
}

/**
* @brief 场景定时触发的服务
* @param no_use 未使用
* @return 无
*/
void *scene_time_trigger(void *no_use)
{
	TIME_SCENE time_scene[MAX_SCENE_NUM];
	INT32 i = 0;
	time_t time_now = 0;
	struct tm tm_now;
	INT32 hour = 0;
	INT32 min = 0;
	INT32 year = 0;
	INT32 mon = 0;
	INT32 mday = 0;
	BOOL wday[7] = {0};

	memset(time_scene, 0, sizeof(time_scene));
	
	FOREVER
	{
		time_now = time(NULL);
		localtime_r(&time_now, &tm_now);
		
		for (i = 0; i < MAX_SCENE_NUM; i++)
		{
			time_scene[i].exsit = FALSE;
		}

		if (get_scene_time_info(time_scene) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "get_scene_time_info failed\n");
			sleep(2);
			continue;
		}

		DEBUG_PRINT(DEBUG_INFO, "time now: %d-%d-%d %d:%d:%d\n", tm_now.tm_year, tm_now.tm_mon, 
		tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

		for (i = 0; i < MAX_SCENE_NUM; i++)
		{
			if (time_scene[i].exsit)
			{
				get_trigger_wday(time_scene[i].trigger_wday, wday);
				if (sscanf(time_scene[i].trigger_date, "%d-%d-%d", &year, &mon, &mday) == 3)
				{
					DEBUG_PRINT(DEBUG_INFO, "scene %d trigger_date: %d-%d-%d\n", 
					i + 1, year, mon, mday);
				}
				else
				{
					year = mon = mday = 0;
					DEBUG_PRINT(DEBUG_INFO, "scene %d no trigger_date: %s\n", i + 1, 
					time_scene[i].trigger_date);
				}
				hour = (time_scene[i].trigger_time >> 8) & 0xFF;
				min = time_scene[i].trigger_time & 0xFF;
				if (!time_scene[i].is_triggered && tm_now.tm_hour == hour && tm_now.tm_min == min 
					&& ((tm_now.tm_year+1900 == year && tm_now.tm_mon+1 == mon && tm_now.tm_mday == mday) || wday[tm_now.tm_wday]))
				{
					DEBUG_PRINT(DEBUG_NOTICE, "scene %d time triggered\n", i + 1);
					trigger_scene_by_module(i + 1);
					time_scene[i].is_triggered = TRUE;
				}

				if (time_scene[i].is_triggered && (tm_now.tm_hour != hour || tm_now.tm_min != min))
				{
					DEBUG_PRINT(DEBUG_NOTICE, "scene %d time trigger flag clear\n", i + 1);
					time_scene[i].is_triggered = FALSE;
				}
			}
			else // 对于不存在的场景需要即时清除触发标志
			{
				time_scene[i].is_triggered = FALSE;
			}
		}

		sleep(5);
	}
}

