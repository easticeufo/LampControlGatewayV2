      
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
#define CONFIG_VERSION CONFIG_VERSION_20170114 // 配置参数版本以日期形式定义
#define MAX_USER_NUM 10 // 最大用户数

/**		  
 * @brief 用户信息
 */
typedef struct
{
	INT8 username[48]; // 不加密的用户名
	INT8 password[48]; // MD5加密后的密码
	UINT32 permission; // 用户权限，暂未使用
}USER_INFO;

/**
 * @brief 配置参数头信息
 */
typedef struct
{
	INT8 flag_str[32]; // 配置参数头部标志字符串，为CONFIG_HEAD_STRING
	UINT32 version; // 配置参数版本
	INT32 config_length; // 配置结构体总长度
	UINT8 res[40];
}CONFIG_HEAD;

/**
 * @brief 设备配置信息结构体
 */
typedef struct
{
	CONFIG_HEAD head; // 配置参数头信息
	USER_INFO user_info[MAX_USER_NUM]; // 用户信息
	time_t manufacture_time; // 设备出厂时间
}DEVICE_CONFIG;

extern DEVICE_CONFIG *get_dev_config(void);
extern INT32 read_config_from_file(void);
extern INT32 write_config_to_file(void);
extern void set_default_config(void);

#endif

