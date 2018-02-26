      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-7-13
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "dev_config.h"

#define CONFIG_FILE "devCfg.bin"

static DEVICE_CONFIG device_config;

/**		  
 * @brief �������ò����ṹ��ָ��
 * @param ��
 * @return �������ò����ṹ��ָ��
 */
DEVICE_CONFIG *get_dev_config(void)
{
	return &device_config;
}

static void patch_config_20160713(void)
{
	device_config.head.version = CONFIG_VERSION_20170114;
	device_config.manufacture_time = time(NULL);
}

/**		  
 * @brief �������ļ��ж�ȡ���ò�����ȫ�ֽṹ����
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
 */
INT32 read_config_from_file(void)
{
	INT32 fd = 0;
	CONFIG_HEAD *p_config_head = (CONFIG_HEAD *)&device_config;
	INT32 len = 0;

	memset(&device_config, 0, sizeof(DEVICE_CONFIG));
	
	/* �������ļ� */
	fd = open(CONFIG_FILE, O_RDWR);
	if (ERROR == fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open %s failed:%s\n", CONFIG_FILE, strerror(errno));
		return ERROR;
	}

	/* ��ȡ����ͷ��Ϣ */
	if (readn(fd, p_config_head, sizeof(CONFIG_HEAD)) != sizeof(CONFIG_HEAD))
	{
		DEBUG_PRINT(DEBUG_ERROR, "readn failed\n");
		SAFE_CLOSE(fd);
		return ERROR;
	}

	/* У��ͷ����Ϣ */
	p_config_head->flag_str[sizeof(p_config_head->flag_str) - 1] = '\0';
	if (strcmp(p_config_head->flag_str, CONFIG_HEAD_STRING) != 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "config head string error! flag_str=%s\n", p_config_head->flag_str);
		SAFE_CLOSE(fd);
		return ERROR;
	}

	/* ��ȡ���ò���������Ϣ */
	len = MIN(sizeof(DEVICE_CONFIG), p_config_head->config_length) - sizeof(CONFIG_HEAD);
	if (readn(fd, &device_config.user_info, len) != len)
	{
		DEBUG_PRINT(DEBUG_ERROR, "readn failed\n");
		SAFE_CLOSE(fd);
		return ERROR;
	}

	/* ���ò����汾��һ��ʱ��Ҫ��ת������ */
	if (p_config_head->version != CONFIG_VERSION)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "config version is not same! version:0x%08X,0x%08X\n"
			, p_config_head->version, CONFIG_VERSION);
		
		if (p_config_head->version == CONFIG_VERSION_20160713)
		{
			patch_config_20160713();
		}

		/* ���겹����д�������ļ� */
		p_config_head->config_length = sizeof(DEVICE_CONFIG);
		lseek(fd, 0, SEEK_SET);
		writen(fd, &device_config, sizeof(DEVICE_CONFIG));
	}
	
	SAFE_CLOSE(fd);

	return OK;
}

/**		  
 * @brief �����ò���д�������ļ�
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
 */
INT32 write_config_to_file(void)
{
	INT32 fd = 0;
	CONFIG_HEAD *p_config_head = (CONFIG_HEAD *)&device_config;

	/* �����������ļ� */
	fd = open(CONFIG_FILE, O_WRONLY | O_CREAT, 0777);
	if (ERROR == fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open failed:%s\n", strerror(errno));
		return ERROR;
	}

	p_config_head->version = CONFIG_VERSION;
	p_config_head->config_length = sizeof(DEVICE_CONFIG);
	
	writen(fd, &device_config, sizeof(DEVICE_CONFIG));
	SAFE_CLOSE(fd);

	return OK;
}

/**		  
 * @brief ���ò�������ΪĬ�ϲ���
 * @param ��
 * @return ��
 */
void set_default_config(void)
{
	CONFIG_HEAD *p_config_head = (CONFIG_HEAD *)&device_config;
	
	memset(&device_config, 0, sizeof(DEVICE_CONFIG));
	strcpy(p_config_head->flag_str, CONFIG_HEAD_STRING);
	p_config_head->version = CONFIG_VERSION;
	p_config_head->config_length = sizeof(DEVICE_CONFIG);

	strcpy(device_config.user_info[0].username, "admin");
	strcpy(device_config.user_info[0].password, "d3ae57b6b6869a4d3f86cae441a592dd"); // "adminhr"��MD5����ֵ
	device_config.user_info[0].permission = 0;

	strcpy(device_config.user_info[1].username, "user1");
	strcpy(device_config.user_info[1].password, "f379eaf3c831b04de153469d1bec345e"); // "666666"��MD5����ֵ
	device_config.user_info[1].permission = 1;

	strcpy(device_config.user_info[2].username, "user2");
	strcpy(device_config.user_info[2].password, "21218cca77804d2ba1922c33e0151105"); // "888888"��MD5����ֵ
	device_config.user_info[2].permission = 2;

	device_config.manufacture_time = 0;
	
	return;
}

