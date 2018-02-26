      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-5-31
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#ifndef _DATABASE_H_
#define _DATABASE_H_

#define BIND_NONE 0
#define BIND_LAMP 1
#define BIND_SCENE 2

#define MAX_SCENE_NUM 100 // 用于定时触发的最大场景数
#define MAX_SCENE_LAMP_NUM 100 // 单场景中最大灯的数目
#define MAX_LAMP_MODULE 256

/**		  
 * @brief 单个按键的信息
 */
typedef struct
{
	INT32 capa;
	INT8 name[210];
	INT32 bind_type;
	INT32 lamp_module_id;
	INT32 lamp_bit_id;
	INT32 scene_number;
}KEY_INFO;

/**		  
 * @brief 单个灯的信息
 */
typedef struct
{
	INT32 capa;
	INT32 state;
	INT8 name[210];
}LAMP_INFO;

/**		  
 * @brief 场景中的灯信息
 */
typedef struct
{
	INT32 lamp_module_id;
	INT32 lamp_bit_id;
	INT32 lamp_state;
}SCENE_LAMP_INFO;

/**		  
 * @brief 单个场景的信息
 */
typedef struct
{
	INT32 number;
	INT8 name[210];
	INT32 trigger_time;
	SCENE_LAMP_INFO lamp[MAX_SCENE_LAMP_NUM];
}SCENE_INFO;

/**		  
 * @brief 定时触发场景的信息
 */
typedef struct
{
	INT32 trigger_time;
	INT8 trigger_date[16];
	INT8 trigger_wday[16];
	BOOL is_triggered;
	BOOL exsit;
}TIME_SCENE;

typedef struct
{
	UINT8 id;
	UINT16 state;
}LAMP_MODULE;

extern INT32 database_init(void);
extern INT32 database_close(void);
extern INT32 begin_transaction(void);
extern INT32 commit_transaction(void);
extern INT32 rollback_transaction(void);
extern INT32 clear_device_table(void);
extern INT32 change_device_to_lamp(INT32 module_id, INT32 io_num, INT32 state);
extern INT32 change_device_to_key(INT32 module_id, INT32 key_num);
extern INT32 delete_device(UINT8 module_id);
extern INT32 clear_bind_and_scene(void);
extern INT32 change_device_state(INT32 module_id, INT32 state);
extern INT32 get_lamp_info(UINT8 lamp_module_id, INT32 lamp_bit_id, LAMP_INFO *p_lamp_info);
extern INT32 get_lamp_list(INT8 *p_buff, INT32 buff_len);
extern INT32 set_lamp_name(UINT8 lamp_module_id, INT32 lamp_bit_id, const INT8 *p_lamp_name);
extern INT32 set_lamp_state_to_db(UINT8 lamp_module_id, INT32 lamp_bit_id, INT32 state);
extern INT32 create_new_scene(SCENE_INFO *p_scene_info);
extern INT32 find_and_create_scene(INT32 number, INT32 *p_trigger_time, INT8 *p_name, INT32 len, INT8 *p_date, INT32 date_len, INT8 *p_wday, INT32 wday_len);
extern INT32 update_scene_param(INT32 number, INT32 trigger_time, const INT8 *p_name, const INT8 *p_date, const INT8 *p_wday);
extern INT32 delete_scene_lamp(INT32 number);
extern INT32 add_scene_lamp(INT32 number, UINT8 lamp_module_id, INT32 lamp_bit_id, INT32 lamp_state);
extern INT32 get_scene_list(INT8 *p_buff, INT32 buff_len);
extern INT32 get_scene_info(INT32 number, INT8 *p_buff, INT32 buff_len);
extern INT32 delete_scene_info(INT32 number);
extern INT32 get_scene_time_info(TIME_SCENE *scene_array);
extern INT32 get_key_list(INT8 *p_buff, INT32 buff_len);
extern INT32 get_key_info(UINT8 key_module_id, INT32 key_bit_id, KEY_INFO *p_key_info);
extern INT32 set_key_info(UINT8 key_module_id, INT32 key_bit_id, const KEY_INFO *p_key_info);
extern INT32 set_lamp_module_state_to_db(UINT8 lamp_module_id, INT32 state);
extern INT32 get_lamp_module_state(UINT8 module_id, UINT16 *p_state);
extern INT32 trigger_scene_by_module(INT32 scene_number);
extern INT32 add_key_module(INT32 module_id, INT32 key_num);
extern INT32 add_lamp_module(INT32 module_id, INT32 io_num, INT32 state);
extern INT32 get_lamp_module(LAMP_MODULE *p_lamp_module, INT32 module_num);
extern INT32 set_all_lamp_module_state(BOOL on);

#endif

