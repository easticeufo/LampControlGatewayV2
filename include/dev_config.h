      
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

#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#define CONFIG_HEAD_STRING "LampControlConfig"
#define CONFIG_VERSION_20160713 0x20160713
#define CONFIG_VERSION_20170114 0x20170114
#define CONFIG_VERSION CONFIG_VERSION_20170114 // ���ò����汾��������ʽ����
#define MAX_USER_NUM 10 // ����û���

/**		  
 * @brief �û���Ϣ
 */
typedef struct
{
	INT8 username[48]; // �����ܵ��û���
	INT8 password[48]; // MD5���ܺ������
	UINT32 permission; // �û�Ȩ�ޣ���δʹ��
}USER_INFO;

/**
 * @brief ���ò���ͷ��Ϣ
 */
typedef struct
{
	INT8 flag_str[32]; // ���ò���ͷ����־�ַ�����ΪCONFIG_HEAD_STRING
	UINT32 version; // ���ò����汾
	INT32 config_length; // ���ýṹ���ܳ���
	UINT8 res[40];
}CONFIG_HEAD;

/**
 * @brief �豸������Ϣ�ṹ��
 */
typedef struct
{
	CONFIG_HEAD head; // ���ò���ͷ��Ϣ
	USER_INFO user_info[MAX_USER_NUM]; // �û���Ϣ
	time_t manufacture_time; // �豸����ʱ��
}DEVICE_CONFIG;

extern DEVICE_CONFIG *get_dev_config(void);
extern INT32 read_config_from_file(void);
extern INT32 write_config_to_file(void);
extern void set_default_config(void);

#endif

