
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-6-1
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "appweb.h"
#include "database.h"
#include "cJSON.h"
#include "pack.h"
#include "dev_config.h"

#define UPGRADE_DIR "/home/userapp" // 程序文件存放目录
#define MAX_LOGIN_NUM 32 // 最大用户登陆数

extern INT32 ctrl_sock_init(void);
extern INT32 websocket_handler_init(Http *http);
extern void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
extern INT32 set_lamp_state(UINT8 lamp_module_id, INT32 lamp_bit_id, INT32 state);

/**		  
 * @brief 设备升级缓冲区，同时只能有一个http连接可以进行设备升级
 */
typedef struct
{
	UINT8 *p_buff; // 存放firmware文件的缓冲区
	INT32 buff_len; // 升级缓冲区大小
	INT32 end_idx; // 当前已经存放的升级包的结尾数据位置+1
	HttpConn *conn; // 拥有当前升级缓冲区的http连接
}UPGRADE_BUFF;

/**		  
 * @brief 用户登陆认证信息
 */
typedef struct
{
	INT8 ip[16]; // 登录的客户端IP地址
	INT8 username[48]; // 登录的用户名
	struct timeval login_time; // 登录时间
	INT32 random_num; // 登录时生成的随机数
	INT8 session_id[36]; // 根据ip username login_time random_num生成的会话ID用于命令认证
	time_t last_action_time; // 上一次操作的时间，用户超出最大用户限制时选择删除的用户
}LOGIN_INFO;

static UPGRADE_BUFF upgrade_buff = {NULL, 0, 0, NULL};
static LOGIN_INFO g_login_info[MAX_LOGIN_NUM];

void print_login_info(void)
{
	INT32 i = 0;
	struct tm login_time_tm;
	struct tm last_action_tm;
	INT8 time_str[32] = {0};

	printf("================================\n");
	printf("Login Client Info:\n");
	printf("================================\n");
	
	for (i = 0; i < MAX_LOGIN_NUM; i++)
	{
		if (g_login_info[i].username[0] != '\0')
		{
			printf("login ip: %s\n", g_login_info[i].ip);
			printf("login username: %s\n", g_login_info[i].username);
			localtime_r(&g_login_info[i].login_time.tv_sec, &login_time_tm);
			strftime(time_str, sizeof(time_str), "%FT%T%z", &login_time_tm);
			printf("login time: %s(sec=%ld, usec=%ld)\n", time_str
				, g_login_info[i].login_time.tv_sec, g_login_info[i].login_time.tv_usec);
			printf("login random number: %d\n", g_login_info[i].random_num);
			localtime_r(&g_login_info[i].last_action_time, &last_action_tm);
			strftime(time_str, sizeof(time_str), "%FT%T%z", &last_action_tm);
			printf("last action time: %s(sec=%ld)\n", time_str, g_login_info[i].last_action_time);
			printf("================================\n");
		}
	}
	
	return;
}

/**		  
 * @brief		设置登录用户信息，计算出对应的session id
 * @param[in] p_ipaddr 登录的客户端IP地址
 * @param[in] p_username 登录用户名
 * @return	返回对应的登录信息结构体指针
 */
static LOGIN_INFO *set_login_info(const INT8 *p_ipaddr, const INT8 *p_username)
{
	INT32 i = 0;
	INT8 data[128] = {0};
	INT32 len = 0;
	UINT8 md5_result[16] = {0};
	LOGIN_INFO login_info;
	time_t min_last_action_time = 0;
	INT32 idx = 0;
	
	if (NULL == p_ipaddr || NULL == p_username)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return NULL;
	}

	/* 设置登录信息 */
	memset(&login_info, 0, sizeof(LOGIN_INFO));
	snprintf(login_info.ip, sizeof(login_info.ip), "%s", p_ipaddr);
	snprintf(login_info.username, sizeof(login_info.username), "%s", p_username);
	gettimeofday(&login_info.login_time, NULL);
	login_info.random_num = random();
	len = snprintf(data, sizeof(data), "%s%s%ld%ld%d", p_ipaddr, p_username
		, login_info.login_time.tv_sec, login_info.login_time.tv_usec
		, login_info.random_num);
	md5((UINT8 *)data, len, md5_result);
	for (i = 0; i < sizeof(md5_result); i++)
	{
		sprintf(login_info.session_id + 2*i, "%02x", md5_result[i]);
	}
	login_info.last_action_time = login_info.login_time.tv_sec;

	/* 首先选择一个空闲的登录信息，如果都满，则从所有登录信息中选择最长时间未操作的进行登录 */
	for (i = 0; i < MAX_LOGIN_NUM; i++)
	{
		if (g_login_info[i].username[0] == '\0')
		{
			idx = i;
			break;
		}
		else
		{
			if (0 == min_last_action_time)
			{
				min_last_action_time = g_login_info[i].last_action_time;
			}
			
			if (min_last_action_time > g_login_info[i].last_action_time)
			{
				min_last_action_time = g_login_info[i].last_action_time;
				idx = i;
			}
		}
	}
	memcpy(&g_login_info[idx], &login_info, sizeof(LOGIN_INFO));

	return &g_login_info[idx];
}

static INT32 api_login_process(HttpConn *conn, INT8 *p_body_buff, INT32 buff_len)
{
	DEVICE_CONFIG *p_dev_config = get_dev_config();
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	INT8 username[48];
	INT8 password[48];
	INT32 i = 0;
	LOGIN_INFO *p_login_info = NULL;
	
	if (NULL == conn || NULL == conn->rx || NULL == conn->rx->method 
		|| NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(conn->rx->method, "POST") == 0)
	{
		if (p_dev_config->user_info[0].username[0] == '\0' 
			|| p_dev_config->user_info[0].password[0] == '\0')
		{
			DEBUG_PRINT(DEBUG_NOTICE, "device not active\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"device not active!\"}");
			return OK;
		}

		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"json data format error!\"}");
			return OK;
		}

		json_item = cJSON_GetObjectItem(json, "username");
		if (json_item != NULL)
		{
			snprintf(username, sizeof(username), "%s", json_item->valuestring);
		}

		json_item = cJSON_GetObjectItem(json, "password");
		if (json_item != NULL)
		{
			snprintf(password, sizeof(password), "%s", json_item->valuestring);
		}
		cJSON_Delete(json);

		if ('\0' == username[0] || '\0' == password[0])
		{
			DEBUG_PRINT(DEBUG_NOTICE, "no username or password\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"no username or password!\"}");
			return OK;
		}
		
		for (i = 0; i < MAX_USER_NUM; i++)
		{
			if (strcmp(p_dev_config->user_info[i].username, username) == 0 
				&& strcmp(p_dev_config->user_info[i].password, password) == 0)
			{
				if (0 == p_dev_config->manufacture_time && (1 == i || 2 == i)) // user1或user2首次登陆时设置出厂时间
				{
					p_dev_config->manufacture_time = time(NULL);
					if (write_config_to_file() != OK)
					{
						DEBUG_PRINT(DEBUG_ERROR, "write_config_to_file failed\n");
					}
				}
				
				if (1 == i) // 有效期1年
				{
					if (time(NULL) - p_dev_config->manufacture_time > 365 * 24 * 60 * 60)
					{
						DEBUG_PRINT(DEBUG_NOTICE, "exceed use time\n");
						snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"exceed 1 year!\"}");
						return OK;
					}
				}

				if (2 == i) // 有效期2年
				{
					if (time(NULL) - p_dev_config->manufacture_time > 2 * 365 * 24 * 60 * 60)
					{
						DEBUG_PRINT(DEBUG_NOTICE, "exceed use time\n");
						snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"exceed 2 year!\"}");
						return OK;
					}
				}
				
				p_login_info = set_login_info(conn->ip, username);

				if (p_login_info != NULL)
				{
					httpSetCookie(conn, "session_id", p_login_info->session_id, "/api", "", 0, 0);
					httpSetCookie(conn, "session_id", p_login_info->session_id, "/websocket", "", 0, 0);
					snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
				}
				else
				{
					snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"exceed max login number!\"}");
				}
				
				return OK;
			}
		}

		DEBUG_PRINT(DEBUG_NOTICE, "username or password is wrong\n");
		snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"username or password is wrong!\"}");
		return OK;
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", conn->rx->method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_logout_process(HttpConn *conn, INT8 *p_body_buff, INT32 buff_len)
{
	INT32 i = 0;
	const INT8 *p_session_id = NULL;
	
	if (NULL == conn || NULL == conn->rx || NULL == conn->rx->method 
		|| NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(conn->rx->method, "POST") == 0)
	{
		p_session_id = httpGetCookie(conn, "session_id");
		if (NULL == p_session_id)
		{
			DEBUG_PRINT(DEBUG_NOTICE, "no session id in Cookie\n");
			
		}
		else
		{
			for (i = 0; i < MAX_LOGIN_NUM; i++)
			{
				if (g_login_info[i].username[0] != '\0' 
					&& strcmp(p_session_id, g_login_info[i].session_id) == 0)
				{
					httpSetCookie(conn, "session_id", p_session_id, "/api", "", -1, 0); // 删除cookie
					httpSetCookie(conn, "session_id", p_session_id, "/websocket", "", -1, 0); // 删除cookie

					memset(&g_login_info[i], 0, sizeof(LOGIN_INFO));

					DEBUG_PRINT(DEBUG_NOTICE, "delete matched session id successfully\n");
					break;
				}
			}
		}

	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", conn->rx->method);
		return ERROR;
	}

	snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
	return OK;
}

/**		  
 * @brief		验证用户信息
 * @param[in] conn 当前appweb连接
 * @return	通过验证返回OK，-1-未登录，-2权限不足
 */
INT32 check_auth_permission(HttpConn *conn)
{
	const INT8 *p_session_id = NULL;
	INT32 i = 0;
	DEVICE_CONFIG *p_dev_config = get_dev_config();

	if (NULL == conn || NULL == conn->ip)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return -1;
	}

	p_session_id = httpGetCookie(conn, "session_id");

	if (NULL == p_session_id)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "no session id in Cookie\n");
		return -1;
	}

	for (i = 0; i < MAX_LOGIN_NUM; i++)
	{
		if (g_login_info[i].username[0] != '\0' 
			&& strcmp(p_session_id, g_login_info[i].session_id) == 0
			&& strcmp(conn->ip, g_login_info[i].ip) == 0)
		{
			g_login_info[i].last_action_time = time(NULL);
			break;
		}
	}
	
	if (i >= MAX_LOGIN_NUM)
	{
		DEBUG_PRINT(DEBUG_WARN, "no matched session id\n");
		return -1;
	}
	
	// 只有第一个用户可以修改时间或密码
	if ((strcmp(conn->rx->uri, "/api/config/time") == 0 || strstr(conn->rx->uri, "/api/config/users/") != NULL)
		&& strcmp(conn->rx->method, "PUT") == 0
		&& strcmp(g_login_info[i].username, p_dev_config->user_info[0].username) != 0)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "no permission to config time\n");
		return -2;
	}

	return OK;
}

/**		  
 * @brief		 获取uri中当前这一层的字符串，在buff中返回，并且返回下一层的起始位置
 * @param[in]  uri 当前层的uri起始位置，一般以'/'打头
 * @param[out] buff 用于返回当前这一层的字符串的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return	 返回下一层的起始位置，如果没有下一层则返回NULL
 */
static INT8 *parse_uri_layer(const INT8 *uri, INT8 *buff, INT32	buff_len)
{
	INT8 *ptr = NULL;
	INT32 len = 0;

	if (NULL == uri || NULL == buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return NULL;
	}
	
	if ('/' == uri[0])
	{
		uri++;
	}

	ptr = strchr(uri, '/');
	if (ptr != NULL)
	{
		len = ptr - uri;
		if (len >= buff_len)
		{
			memcpy(buff, uri, buff_len - 1);
			buff[buff_len - 1] = '\0';
		}
		else
		{
			memcpy(buff, uri, len);
			buff[len] = '\0';
		}
	}
	else // 已经是最后一层
	{
		strncpy(buff, uri, buff_len - 1);
		buff[buff_len - 1] = '\0';
	}
	
	return ptr;
}

static INT32 api_lights_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		return get_lamp_list(p_body_buff, buff_len);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_lights_all_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len, const INT8 *action)
{
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "POST") == 0)
	{
		if (strcmp(action, "on") == 0) // 全开
		{
			return set_all_lamp_module_state(TRUE);
		}
		else if (strcmp(action, "off") == 0) // 全关
		{
			return set_all_lamp_module_state(FALSE);
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_lights_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 id = -1;
	UINT8 lamp_module_id = 0;
	INT32 lamp_bit_id = 0;
	LAMP_INFO lamp_info;
	INT8 type_str[8] ={0};
	INT8 state_str[8] ={0};
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	if (p_next_layer != NULL)
	{
		DEBUG_PRINT(DEBUG_WARN, "not found\n");
		return ERROR;
	}

	if (strcmp(layer_str, "on") == 0 || strcmp(layer_str, "off") == 0)
	{
		return api_lights_all_process(method, p_body_buff, buff_len, layer_str);
	}

	id = atoi(layer_str);
	lamp_module_id = (UINT8)((id >> 8) & 0xFF);
	lamp_bit_id = id & 0xFF;
	DEBUG_PRINT(DEBUG_NOTICE, "api_lights_parse light id=%d, lamp_module_id=%d, lamp_bit_id=%d\n"
		, id, lamp_module_id, lamp_bit_id);
	if (id < 0 || id > 0xFFFF)
	{
		DEBUG_PRINT(DEBUG_WARN, "light id out of range\n");
		return ERROR;
	}

	if (get_lamp_info(lamp_module_id, lamp_bit_id, &lamp_info) != OK)
	{
		DEBUG_PRINT(DEBUG_WARN, "find lamp failed!\n");
		return ERROR;
	}

	if (4 == lamp_info.capa)
	{
		strcpy(type_str, "4IO");
	}
	else if (8 == lamp_info.capa)
	{
		strcpy(type_str, "8IO");
	}
	else if (12 == lamp_info.capa)
	{
		strcpy(type_str, "12IO");
	}
	else
	{
		strcpy(type_str, "UNKNOWN");
	}

	if (strcmp(method, "GET") == 0)
	{
		/* 构造返回的JSON格式数据 */
		if (0 == lamp_info.state)
		{
			strcpy(state_str, "false");
		}
		else
		{
			strcpy(state_str, "true");
		}

		snprintf(p_body_buff, buff_len, "{\"type\":\"%s\",\"on\":%s,\"name\":\"%s\"}"
			, type_str, state_str, lamp_info.name);
		DEBUG_PRINT(DEBUG_NOTICE, "get_lamp_info:\n%s\n", p_body_buff);
		
		return OK;
	}
	else if (strcmp(method, "PUT") == 0)
	{
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"json data format error!\"}");
			return OK;
		}

		json_item = cJSON_GetObjectItem(json, "on");
		if (json_item != NULL)
		{
			if (set_lamp_state(lamp_module_id, lamp_bit_id, json_item->valueint) != OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "set lamp state error\n");
				return ERROR;
			}

			lamp_info.state = json_item->valueint;

			if (0 == lamp_info.state)
			{
				strcpy(state_str, "false");
			}
			else
			{
				strcpy(state_str, "true");
			}
		}

		json_item = cJSON_GetObjectItem(json, "name");
		if (json_item != NULL)
		{
			if (set_lamp_name(lamp_module_id, lamp_bit_id, json_item->valuestring) != OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "set lamp name error\n");
				return ERROR;
			}

			snprintf(lamp_info.name, sizeof(lamp_info.name), "%s", json_item->valuestring);
		}

		cJSON_Delete(json);

		snprintf(p_body_buff, buff_len, "{\"id\":%d,\"type\":\"%s\",\"on\":%s,\"name\":\"%s\"}"
			,id , type_str, state_str, lamp_info.name);
		
		return OK;
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
}

static INT32 api_scenes_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	SCENE_INFO new_scene;
	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		return get_scene_list(p_body_buff, buff_len);
	}
	else if (strcmp(method, "POST") == 0)
	{
		if (create_new_scene(&new_scene) == ERROR)
		{
			DEBUG_PRINT(DEBUG_ERROR, "create new scene error\n");
        	return ERROR;
		}

		snprintf(p_body_buff, buff_len
			, "{\"id\":%d,\"name\":\"\",\"trigger_time\":{\"date\":\"\",\"wday\":[],\"time\":\"\"},\"lights\":[]}"
			, new_scene.number);
		return OK;
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
}

static INT32 api_scenes_id_trigger_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len, INT32 number)
{
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "POST") == 0)
	{
		if (trigger_scene_by_module(number) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "trigger_scene_by_module failed\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"trigger scene error!\"}");
		}
		else
		{
			snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}


static INT32 api_scenes_id_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len, INT32 id)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	if (p_next_layer != NULL)
	{
		DEBUG_PRINT(DEBUG_WARN, "not found\n");
		return ERROR;
	}
	
	if (strcmp(layer_str, "trigger") == 0)
	{
		return api_scenes_id_trigger_process(method, p_body_buff, buff_len, id);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "layer_str=%s not found\n", layer_str);
		return ERROR;
	}
}

static INT32 api_scenes_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 number = -1;
	INT8 scene_name[210] = {0};
	INT8 scene_date[16] = {0};
	INT8 scene_wday[16] = {0};
	INT32 scene_time = -1;
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	cJSON *trigger_time_item = NULL;
	cJSON *wday_item = NULL;
	INT32 wday_num = 0;
	cJSON *lights_item = NULL;
	cJSON *lamp_item = NULL;
	INT32 hour = -1;
	INT32 min = -1;
	INT32 lights_num = 0;
	INT32 i = 0;
	UINT8 lamp_module_id = 0;
	INT32 lamp_bit_id = 0;
	INT32 lamp_state = 0;
	INT32 len = 0;
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));

	number = atoi(layer_str);
	DEBUG_PRINT(DEBUG_NOTICE, "api_scenes_parse number=%d\n", number);
	if (number < 0)
	{
		DEBUG_PRINT(DEBUG_WARN, "scene number out of range\n");
		return ERROR;
	}

	if (p_next_layer != NULL) // /api/scenes/<id>/xxx
	{
		return api_scenes_id_parse(method, p_next_layer, p_body_buff, buff_len, number);
	}

	if (strcmp(method, "GET") == 0)
	{
		return get_scene_info(number, p_body_buff, buff_len);
	}
	else if (strcmp(method, "PUT") == 0)
	{
		/* 首先创建场景并获取场景的相关参数 */
		if (find_and_create_scene(number, &scene_time, scene_name, sizeof(scene_name), 
			scene_date, sizeof(scene_date), scene_wday, sizeof(scene_wday)) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "find_and_create_scene failed\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"create scene failed!\"}");
			return OK;
		}
		
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"json data format error!\"}");
			return OK;
		}

		json_item = cJSON_GetObjectItem(json, "name");
		if (json_item != NULL)
		{
			snprintf(scene_name, sizeof(scene_name), "%s", json_item->valuestring);
		}

		trigger_time_item = cJSON_GetObjectItem(json, "trigger_time");
		if (trigger_time_item != NULL)
		{
			json_item = cJSON_GetObjectItem(trigger_time_item, "time");
			if (json_item != NULL)
			{
				sscanf(json_item->valuestring, "%d:%d", &hour, &min);
				if (hour >= 0 && hour < 24 && min >= 0 && min < 60)
				{
					scene_time = (hour << 8) | min;
				}
				else
				{
					scene_time = -1;
				}
			}

			json_item = cJSON_GetObjectItem(trigger_time_item, "date");
			if (json_item != NULL)
			{
				snprintf(scene_date, sizeof(scene_date), "%s", json_item->valuestring);
			}

			wday_item = cJSON_GetObjectItem(trigger_time_item, "wday");
			if (wday_item != NULL)
			{
				memset(scene_wday, 0, sizeof(scene_wday));
				wday_num = cJSON_GetArraySize(wday_item);
				for (i = 0; i < wday_num; i++)
				{
					json_item = cJSON_GetArrayItem(wday_item, i);
					len += snprintf(scene_wday + len, sizeof(scene_wday) - len, "%d,", json_item->valueint);
				}
				if (wday_num != 0) // 去除最后一个逗号
				{
					scene_wday[len - 1] = '\0';
				}
			}
		}

		/* 更新场景名称和时间参数 */
		if (update_scene_param(number, scene_time, scene_name, scene_date, scene_wday) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "find_and_create_scene failed\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"update scene param failed!\"}");
			cJSON_Delete(json);
			return OK;
		}

		lights_item = cJSON_GetObjectItem(json, "lights");
		if (lights_item != NULL)
		{
			begin_transaction();
			/* 删除该场景绑定的所有灯 */
			if (delete_scene_lamp(number) != OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "delete_scene_lamp failed\n");
				snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"delete scene lamp failed!\"}");
				cJSON_Delete(json);
				rollback_transaction();
				return OK;
			}

			lights_num = cJSON_GetArraySize(lights_item);
			for (i = 0; i < lights_num; i++)
			{
				lamp_item = cJSON_GetArrayItem(lights_item, i);
				json_item = cJSON_GetObjectItem(lamp_item, "id");
				if (NULL == json_item)
				{
					DEBUG_PRINT(DEBUG_ERROR, "no scene lamp id\n");
					continue;
				}
				lamp_module_id = (UINT8)((json_item->valueint >> 8) & 0xFF);
				lamp_bit_id = json_item->valueint & 0xFF;

				json_item = cJSON_GetObjectItem(lamp_item, "on");
				if (NULL == json_item)
				{
					DEBUG_PRINT(DEBUG_ERROR, "no scene lamp state\n");
					continue;
				}
				lamp_state = json_item->valueint;

				/* 增加场景关联的一个灯 */
				if (add_scene_lamp(number, lamp_module_id, lamp_bit_id, lamp_state) != OK)
				{
					DEBUG_PRINT(DEBUG_ERROR, "add_scene_lamp failed\n");
					continue;
				}
			}
			commit_transaction();
		}

		cJSON_Delete(json);
		
		return get_scene_info(number, p_body_buff, buff_len);
	}
	else if (strcmp(method, "DELETE") == 0)
	{
		if (delete_scene_info(number) != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "delete_scene_info failed\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"delete scene error!\"}");
			return OK;
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}

	snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
	return OK;
}

static INT32 api_keys_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		return get_key_list(p_body_buff, buff_len);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_keys_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 id = -1;
	UINT8 key_module_id = 0;
	INT32 key_bit_id = 0;
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	cJSON *bind_item = NULL;
	KEY_INFO key_info;
	INT8 bind_info[48] = {0};
	INT8 type_str[8] = {0};
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	memset(&key_info, 0, sizeof(key_info));

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	if (p_next_layer != NULL)
	{
		DEBUG_PRINT(DEBUG_WARN, "not found\n");
		return ERROR;
	}

	id = atoi(layer_str);
	key_module_id = (UINT8)((id >> 8) & 0xFF);
	key_bit_id = id & 0xFF;
	DEBUG_PRINT(DEBUG_NOTICE, "api_keys_parse key id=%d, key_module_id=%d, key_bit_id=%d\n"
		, id, key_module_id, key_bit_id);
	if (id < 0 || id > 0xFFFF)
	{
		DEBUG_PRINT(DEBUG_WARN, "key id out of range\n");
		return ERROR;
	}

	if (get_key_info(key_module_id, key_bit_id, &key_info) != OK)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find key!\n");
		return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		if (4 == key_info.capa)
		{
			strcpy(type_str, "4KEY");
		}
		else if (6 == key_info.capa)
		{
			strcpy(type_str, "6KEY");
		}
		else if (8 == key_info.capa)
		{
			strcpy(type_str, "8KEY");
		}
		else
		{
			strcpy(type_str, "UNKNOWN");
		}
		
		if (BIND_LAMP == key_info.bind_type)
		{
			snprintf(bind_info, sizeof(bind_info)
				, "{\"type\":\"light\",\"id\":%d}", (key_info.lamp_module_id << 8) | key_info.lamp_bit_id);
		}
		else if (BIND_SCENE == key_info.bind_type)
		{
			snprintf(bind_info, sizeof(bind_info)
				, "{\"type\":\"scene\",\"id\":%d}", key_info.scene_number);
		}
		else
		{
			strcpy(bind_info, "{}");
		}
		
		snprintf(p_body_buff, buff_len,  "{\"id\":%d,\"type\":\"%s\",\"name\":\"%s\",\"bind\":%s}"
			,id , type_str, key_info.name, bind_info);
		
		return OK;
	}
	else if (strcmp(method, "PUT") == 0)
	{
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"json data format error!\"}");
			return OK;
		}

		json_item = cJSON_GetObjectItem(json, "name");
		if (json_item != NULL)
		{
			snprintf(key_info.name, sizeof(key_info.name), "%s", json_item->valuestring);
		}

		bind_item = cJSON_GetObjectItem(json, "bind");
		if (bind_item != NULL)
		{
			json_item = cJSON_GetObjectItem(bind_item, "type");
			if (json_item != NULL)
			{
				if (strcmp(json_item->valuestring, "light") == 0)
				{
					key_info.bind_type = BIND_LAMP;
				}
				else if (strcmp(json_item->valuestring, "scene") == 0)
				{
					key_info.bind_type = BIND_SCENE;
				}
				else
				{
					key_info.bind_type = BIND_NONE;
				}
			}
			else
			{
				key_info.bind_type = BIND_NONE;
			}

			json_item = cJSON_GetObjectItem(bind_item, "id");
			if (json_item != NULL)
			{
				if (BIND_LAMP == key_info.bind_type)
				{
					key_info.lamp_module_id = (json_item->valueint >> 8) & 0xFF;
					key_info.lamp_bit_id = json_item->valueint & 0xFF;
				}
				else if (BIND_SCENE == key_info.bind_type)
				{
					key_info.scene_number = json_item->valueint;
				}
				else
				{
					// do nothing
				}
			}
		}

		cJSON_Delete(json);

		(void)set_key_info(key_module_id, key_bit_id, &key_info);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}

	snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
	
	return OK;
}

static INT32 api_config_system_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 build_date[20] = {0};
	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		get_build_date(build_date, sizeof(build_date));
		snprintf(p_body_buff, buff_len, "{\"gwversion\":\"V2.0.0 %s\"}", build_date);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_config_time_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	time_t time_now = time(NULL);
	struct tm tm_now;
	INT8 time_str[64] = {0};
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	INT32 year = 0;
	INT32 month = 0;
	INT32 day = 0;
	INT32 hour = 0;
	INT32 min = 0;
	INT32 sec = 0;
	INT8 tz[8] = {0};
	INT32 ret = 0;
	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		gmtime_r(&time_now, &tm_now);
		strftime(time_str, sizeof(time_str), "%FT%TZ", &tm_now);
		snprintf(p_body_buff, buff_len, "{\"utcTime\":\"%s\"}", time_str);
	}
	else if (strcmp(method, "PUT") == 0)
	{
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"json data format error!\"}");
			return OK;
		}

		json_item = cJSON_GetObjectItem(json, "utcTime");
		if (json_item != NULL)
		{
			sscanf(json_item->valuestring, "%d-%d-%dT%d:%d:%d%s"
				, &year, &month, &day, &hour, &min, &sec, tz);
			if (sec < 0 || sec > 59 || min < 0 || min > 59 || hour < 0 || hour > 23 
				|| day < 1 || day > 31 || month < 1 || month > 12 || year > 2099)
			{
				DEBUG_PRINT(DEBUG_WARN, "time format error\n");
				snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"time format error!\"}");
			}
			else
			{
				snprintf(time_str, sizeof(time_str), "date -s \"%04d-%02d-%02d %02d:%02d:%02d\" -u"
					, year, month, day, hour, min, sec);
				ret = system(time_str); // 设置系统时间
				DEBUG_PRINT(DEBUG_NOTICE, "system() ret=%d\n", ret);
				ret = system("hwclock -w --utc"); // 写入RTC保存
				DEBUG_PRINT(DEBUG_NOTICE, "system() ret=%d\n", ret);
				snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
			}
		}
		cJSON_Delete(json);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

/**		  
 * @brief 获取用户列表，通过JSON格式数据返回
 * @param[out] p_buff 用于返回JSON数据的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return OK-成功，ERROR-失败
 */
static INT32 get_user_list(INT8 *p_buff, INT32 buff_len)
{
	USER_INFO *p_user_info = NULL;
	INT32 i = 0;
	INT32 len = 0;

	if (NULL == p_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_user_info = get_dev_config()->user_info;

	len = snprintf(p_buff, buff_len, "%s", "[");
	for (i = 0; i < MAX_USER_NUM; i++)
	{
		if (p_user_info[i].username[0] != '\0')
		{
			len += snprintf(p_buff + len, buff_len - len, "{\"id\":%d,\"username\":\"%s\",\"level\":%u},"
				, i+1, p_user_info[i].username, p_user_info[i].permission);
			if (len >= buff_len)
			{
				DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
				return ERROR;
			}
		}
	}

	if (1 == len) // 用户列表为空
	{
		len++;
	}
	
	p_buff[len - 1] = ']'; // 将最后','改为']'
	p_buff[len] = '\0';
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_user_list:\n%s\n", p_buff);

	return OK;
}

static INT32 api_config_users_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "GET") == 0)
	{
		return get_user_list(p_body_buff, buff_len);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_config_users_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 id = -1;
	USER_INFO *p_user_info = NULL;
	cJSON *json = NULL;
	cJSON *json_item = NULL;
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	if (p_next_layer != NULL)
	{
		DEBUG_PRINT(DEBUG_WARN, "not found\n");
		return ERROR;
	}

	id = atoi(layer_str);
	DEBUG_PRINT(DEBUG_NOTICE, "api_config_users_parse id=%d\n", id);
	if (id < 1 || id > MAX_USER_NUM)
	{
		DEBUG_PRINT(DEBUG_WARN, "user id out of range\n");
		return ERROR;
	}

	p_user_info = get_dev_config()->user_info;

	if (strcmp(method, "GET") == 0)
	{
		if (p_user_info[id-1].username[0] == '\0')
		{
			DEBUG_PRINT(DEBUG_NOTICE, "user %d is not exist\n", id);
			return ERROR;
		}
		else
		{
			snprintf(p_body_buff, buff_len, "{\"username\":\"%s\",\"level\":%u}"
				, p_user_info[id-1].username, p_user_info[id-1].permission);
			return OK;
		}
	}
	else if (strcmp(method, "PUT") == 0)
	{
		json = cJSON_Parse(p_body_buff);
		if (NULL == json)
		{
			DEBUG_PRINT(DEBUG_ERROR, "cJSON_Parse error:%s\n", cJSON_GetErrorPtr());
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"json data format error!\"}");
			return OK;
		}

		json_item = cJSON_GetObjectItem(json, "username");
		if (json_item != NULL)
		{
			snprintf(p_user_info[id-1].username, sizeof(p_user_info[id-1].username), "%s", json_item->valuestring);
		}

		json_item = cJSON_GetObjectItem(json, "password");
		if (json_item != NULL)
		{
			snprintf(p_user_info[id-1].password, sizeof(p_user_info[id-1].password), "%s", json_item->valuestring);
		}

		json_item = cJSON_GetObjectItem(json, "level");
		if (json_item != NULL)
		{
			p_user_info[id-1].permission = json_item->valueint;
		}

		cJSON_Delete(json);

		if (write_config_to_file() != OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "write_config_to_file failed\n");
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"save config to file failed!\"}");
		}
		else
		{
			snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
		}

		return OK;
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
}

static INT32 api_config_parse(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 ret = 0;
	
	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	if (strcmp(layer_str, "system") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_config_system_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = ERROR;
		}
	}
	else if (strcmp(layer_str, "time") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_config_time_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = ERROR;
		}
	}
	else if (strcmp(layer_str, "users") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_config_users_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = api_config_users_parse(method, p_next_layer, p_body_buff, buff_len);
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "layer_str=%s not found\n", layer_str);
		return ERROR;
	}

	return ret;
}

static INT32 firmware_upgrade(const UINT8 *p_firm_buff, INT32 firm_len)
{
	const FIRMWARE_HEADER *p_firm_header = NULL;
	const UPGRADE_FILE_HEADER *p_file_header = NULL;
	const UINT8 *p_file_buff = NULL;
	INT32 file_num = 0;
	INT32 i = 0;
	INT8 file_path_name[64] = {0};
	INT32 file_fd = 0;
	INT32 ret = 0;
	
	if (NULL == p_firm_buff || firm_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	/* 校验firmware头信息 */
	if (firm_len < sizeof(FIRMWARE_HEADER))
	{
		DEBUG_PRINT(DEBUG_ERROR, "firmware header error: firm_len=%d\n", firm_len);
		return ERROR;
	}
	p_firm_header = (const FIRMWARE_HEADER *)p_firm_buff;
	file_num = p_firm_header->file_num;
	if (p_firm_header->magic_number != PACK_MAGIC_NUMBER || file_num < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "firmware header error: magic_number=0x%x file_num=%d\n"
			, p_firm_header->magic_number, file_num);
		return ERROR;
	}

	/* 校验upgrade file头信息 */
	if (firm_len < sizeof(FIRMWARE_HEADER) + file_num * sizeof(UPGRADE_FILE_HEADER))
	{
		DEBUG_PRINT(DEBUG_ERROR, "upgrade file header error: firm_len=%d\n", firm_len);
		return ERROR;
	}

	ret = system("mount -o remount rw /"); // 网关板的文件系统默认是只读的，这里先将板子文件系统设置为读写
	DEBUG_PRINT(DEBUG_NOTICE, "system() ret=%d\n", ret);

	/* 读取并校验各个文件内容，生成升级文件 */
	p_file_header = (const UPGRADE_FILE_HEADER *)(p_firm_buff + sizeof(FIRMWARE_HEADER));
	for (i = 0; i < file_num; i++)
	{
		if (firm_len < (p_file_header[i].start_offset + p_file_header[i].file_len))
		{
			DEBUG_PRINT(DEBUG_ERROR, "upgrade file error: firm_len=%d, i=%d\n", firm_len, i);
			return ERROR;
		}
		p_file_buff = p_firm_buff + p_file_header[i].start_offset;
		if (checksum_u8(p_file_buff, p_file_header[i].file_len) != p_file_header[i].check_sum)
		{
			DEBUG_PRINT(DEBUG_ERROR, "upgrade file error: checksum_u8 failed, i=%d\n", i);
			return ERROR;
		}
		
		snprintf(file_path_name, sizeof(file_path_name), "%s/%s", UPGRADE_DIR, p_file_header[i].file_name);
		file_fd = open(file_path_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (ERROR == file_fd)
		{
			DEBUG_PRINT(DEBUG_ERROR, "open %s failed:%s\n", file_path_name, strerror(errno));
			return ERROR;
		}

		writen(file_fd, p_file_buff, p_file_header[i].file_len);
		SAFE_CLOSE(file_fd);
	}

	return OK;
}

static INT32 api_upgrade_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "POST") == 0)
	{
		if (upgrade_buff.buff_len != upgrade_buff.end_idx)
		{
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"firmware is incomplete!\"}");
		}
		else
		{
			if (firmware_upgrade(upgrade_buff.p_buff, upgrade_buff.buff_len) == OK)
			{
				snprintf(p_body_buff, buff_len, "{\"success\":true,\"msg\":\"success!\"}");
			}
			else
			{
				snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"upgrade failed!\"}");
			}
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static BOOL system_can_reboot(void)
{
	if (upgrade_buff.conn != NULL) // 系统升级时不能重启
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static INT32 api_reboot_process(const INT8 *method, INT8 *p_body_buff, INT32 buff_len)
{	
	if (NULL == method || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (strcmp(method, "POST") == 0)
	{
		if (!system_can_reboot())
		{
			snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"can't reboot now!\"}");
		}
		else
		{
			if (database_close() != OK)
			{
				snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"database close failed!\"}");
			}
			else
			{
				sync();

				if (reboot(RB_AUTOBOOT) == ERROR) // 成功重启此函数不会返回
				{
					DEBUG_PRINT(DEBUG_ERROR, "reboot failed: %s\n", strerror(errno));
					snprintf(p_body_buff, buff_len, "{\"success\":false,\"msg\":\"reboot failed!\"}");
				}
			}
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "this url do not support method %s\n", method);
		return ERROR;
	}
	
	return OK;
}

static INT32 api_main(const INT8 *method, const INT8 *uri, INT8 *p_body_buff, INT32 buff_len)
{
	INT8 layer_str[16] = {0};
	INT8 *p_next_layer = NULL;
	INT32 ret = 0;

	if (NULL == method || NULL == uri || NULL == p_body_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}
	
	p_next_layer = parse_uri_layer(uri, layer_str, sizeof(layer_str));
	DEBUG_PRINT(DEBUG_NOTICE, "api_main layer_str=%s\n", layer_str);
	if (NULL == p_next_layer)
	{
		DEBUG_PRINT(DEBUG_WARN, "input uri dose not have layer after %s\n", layer_str);
		return ERROR;
	}

	p_next_layer = parse_uri_layer(p_next_layer, layer_str, sizeof(layer_str));
	if (strcmp(layer_str, "lights") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_lights_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = api_lights_parse(method, p_next_layer, p_body_buff, buff_len);
		}
	}
	else if (strcmp(layer_str, "scenes") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_scenes_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = api_scenes_parse(method, p_next_layer, p_body_buff, buff_len);
		}
	}
	else if (strcmp(layer_str, "keys") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_keys_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = api_keys_parse(method, p_next_layer, p_body_buff, buff_len);
		}
	}
	else if (strcmp(layer_str, "config") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = ERROR;
		}
		else
		{
			ret = api_config_parse(method, p_next_layer, p_body_buff, buff_len);
		}
	}
	else if (strcmp(layer_str, "upgrade") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_upgrade_process(method, p_body_buff, buff_len);
			
			/* 升级完成后需要释放升级缓冲区资源 */
			SAFE_FREE(upgrade_buff.p_buff);
			upgrade_buff.buff_len = 0;
			upgrade_buff.end_idx = 0;
			upgrade_buff.conn = NULL;
		}
		else
		{
			ret = ERROR;
		}
	}
	else if (strcmp(layer_str, "reboot") == 0)
	{
		if (NULL == p_next_layer)
		{
			ret = api_reboot_process(method, p_body_buff, buff_len);
		}
		else
		{
			ret = ERROR;
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "layer_str=%s not found\n", layer_str);
		return ERROR;
	}
	
	return ret;
}

static void put_packet_to_upgrade_buff(HttpConn *conn, HttpPacket *packet)
{
	INT8 *ptr = NULL;
	INT32 len = 0;
	
	if (NULL == conn || NULL == packet || NULL == conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return;
	}

	DEBUG_PRINT(DEBUG_NOTICE, "receive upgrade data\n");

	if (NULL == upgrade_buff.conn) // 第一次收到升级数据
	{
		upgrade_buff.conn = conn;
		SAFE_FREE(upgrade_buff.p_buff);
		upgrade_buff.buff_len = conn->rx->length;
		upgrade_buff.p_buff = (UINT8 *)malloc(upgrade_buff.buff_len);
		upgrade_buff.end_idx = 0;
	}
	else
	{
		if (upgrade_buff.conn != conn)
		{
			DEBUG_PRINT(DEBUG_WARN, "can't upgrade: other client is upgrading!\n");
			return;
		}
	}

	len = MIN(httpGetPacketLength(packet), upgrade_buff.buff_len - upgrade_buff.end_idx);
	ptr = httpGetPacketStart(packet);
	if (ptr != NULL && len > 0)
	{
		memcpy(upgrade_buff.p_buff + upgrade_buff.end_idx, ptr, len);
		upgrade_buff.end_idx += len;
	}

	return;
}

/**		  
 * @brief 为了处理/api/upgrade升级功能，其他操作同appweb库中的incoming函数相同
 * @param q http接收的数据队列
 * @param packet 当前收到的数据包
 * @return 无
 */
static void api_incoming(HttpQueue *q, HttpPacket *packet)
{
	if (NULL == q || NULL == q->conn || NULL == q->conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return;
	}

	if (q->nextQ->put)
	{
		httpPutPacketToNext(q, packet);
	}
	else
	{
		/* This queue is the last queue in the pipeline */
		if (httpGetPacketLength(packet) > 0)
		{
			if (packet->flags & HTTP_PACKET_SOLO)
			{
				httpPutForService(q, packet, HTTP_DELAY_SERVICE);
			} 
			else
			{
				if (strcmp(q->conn->rx->uri, "/api/upgrade") == 0) // 升级功能由于升级包很大，所以要做特殊处理
				{
					put_packet_to_upgrade_buff(q->conn, packet);
				}
				else
				{
					httpJoinPacketForService(q, packet, 0);
				}
			}
		} 
		else
		{
			/* Zero length packet means eof */
			httpPutForService(q, packet, HTTP_DELAY_SERVICE);
		}
		HTTP_NOTIFY(q->conn, HTTP_EVENT_READABLE, 0);
	}

	return;
}

static void api_entry(HttpQueue *q)
{
    HttpConn *conn = NULL;
	INT8 http_body[100 * 1024] = {0};
	INT32 len = 0;

	if (NULL == q || NULL == q->conn || NULL == q->conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return;
	}
	
	conn = q->conn;

	if (strcmp(conn->rx->method, "OPTIONS") == 0) // PUT命令跨域访问时会先发送OPTIONS命令
	{
		DEBUG_PRINT(DEBUG_NOTICE, "API OPTIONS!\n");
		httpFinalize(conn);
		return;
	}

	len = httpRead(conn, http_body, sizeof(http_body));

	DEBUG_PRINT(DEBUG_NOTICE, "apiHandler received method=%s uri=%s http body length=%d body content:\n%s\n"
		, conn->rx->method, conn->rx->uri, len, http_body);

	if (strcmp(conn->rx->uri, "/api/login") == 0) // 用户登录命令不需要权限验证，在这里单独处理
	{
		if (api_login_process(conn, http_body, sizeof(http_body)) == OK)
		{
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_OK);
			httpWrite(q, "%s", http_body);
			httpFinalize(conn);
		}
		else
		{
			httpError(conn, HTTP_CODE_NOT_FOUND, "Can't run api process: %s %s", conn->rx->method, conn->rx->uri);
			DEBUG_PRINT(DEBUG_WARN, "api_login_process return ERROR\n");
		}

		return;
	}
	else if (strcmp(conn->rx->uri, "/api/logout") == 0) // 登出命令也在这里单独处理
	{
		if (api_logout_process(conn, http_body, sizeof(http_body)) == OK)
		{
			httpSetHeader(conn, "Content-Type", "application/json");
			httpSetStatus(conn, HTTP_CODE_OK);
			httpWrite(q, "%s", http_body);
			httpFinalize(conn);
		}
		else
		{
			httpError(conn, HTTP_CODE_NOT_FOUND, "Can't run api process: %s %s", conn->rx->method, conn->rx->uri);
			DEBUG_PRINT(DEBUG_WARN, "api_logout_process return ERROR\n");
		}

		return;
	}
	else
	{
		/* 用户权限验证 */
		if (check_auth_permission(conn) == -1)
		{
			httpError(conn, HTTP_CODE_UNAUTHORIZED, "Access Denied. Login required");
			DEBUG_PRINT(DEBUG_NOTICE, "unauthorized!\n");
			return;
		}
		else if (check_auth_permission(conn) == -2)
		{
			httpError(conn, HTTP_CODE_FORBIDDEN, "Insufficient permissions to perform this action!");
			DEBUG_PRINT(DEBUG_NOTICE, "unauthorized!\n");
			return;
		}
	}
	
	if (strcmp(conn->rx->uri, "/api/upgrade") == 0) // 升级命令需要特殊处理
	{
		if (conn != upgrade_buff.conn)
		{
			httpError(conn, HTTP_CODE_FORBIDDEN, "other client is upgrading!");
			DEBUG_PRINT(DEBUG_WARN, "other client is upgrading!\n");
			return;
		}
	}
	
	if (api_main(conn->rx->method, conn->rx->uri, http_body, sizeof(http_body)) == OK)
	{
		httpSetHeader(conn, "Content-Type", "application/json");
		httpSetStatus(conn, HTTP_CODE_OK);
		httpWrite(q, "%s", http_body);
		httpFinalize(conn);
	}
	else
	{
		httpError(conn, HTTP_CODE_NOT_FOUND, "Can't run api process: %s %s", conn->rx->method, conn->rx->uri);
		DEBUG_PRINT(DEBUG_WARN, "api_main return ERROR\n");
	}
	
	return;
}

static INT32 api_handler_init(Http *http)
{
    HttpStage   *handler = NULL;

	if (NULL == http)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	if (ctrl_sock_init() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "ctrl_sock_init failed!\n");
		return ERROR;
	}
	
	handler = httpCreateHandler(http, "apiHandler", NULL);
    if (NULL == handler)
	{
		DEBUG_PRINT(DEBUG_ERROR, "httpCreateHandler failed!\n");
        return ERROR;
    }

	handler->incoming = api_incoming;
    handler->ready = api_entry;
	
    return OK;
}

/**		  
 * @brief appweb服务
 * @param no_use 未使用
 * @return 无
 */
void *web_server(void *no_use)
{
	Mpr *mpr = NULL;
	MaServer *server = NULL;
	MaAppweb *appweb = NULL;
	INT32 ret = 0;

	memset(&g_login_info, 0, sizeof(g_login_info));
	
	mpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD);
	if (NULL == mpr)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mprCreate failed!\n");
		return NULL;
	}

	if (mprStart() < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mprStart failed!\n");
		mprDestroy();
		return NULL;
	}

	appweb = maCreateAppweb();
	if (NULL == appweb)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maCreateAppweb failed!\n");
		mprDestroy();
		return NULL;
	}
	mprAddRoot(appweb);

	server = maCreateServer(appweb, "default");
	if (NULL == server)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maCreateServer failed!\n");
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	if (api_handler_init(MPR->httpService) != OK) // 增加自定义的api处理模块
	{
		DEBUG_PRINT(DEBUG_ERROR, "api_handler_init failed!\n");
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	if (websocket_handler_init(MPR->httpService) != OK) // 增加websocket功能处理模块
	{
		DEBUG_PRINT(DEBUG_ERROR, "websocket_handler_init failed!\n");
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	ret = maParseConfig(server, "appweb.conf", 0); // appweb配置参数在appweb.conf文件中定义
	if (ret != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maParseConfig failed! Mpr error code=%d\n", ret);
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	ret = maStartServer(server);
	if (ret != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "maStartServer failed! Mpr error code=%d\n", ret);
		mprRemoveRoot(appweb);
		mprDestroy();
		return NULL;
	}

	ret = mprServiceEvents(-1, 0); // 正常运行时该函数不会返回
	
	DEBUG_PRINT(DEBUG_ERROR, "Stopping Appweb! mprServiceEvents ret=%d\n", ret);
	maStopServer(server);
	mprRemoveRoot(appweb);
	mprDestroy();

	return NULL;
}

