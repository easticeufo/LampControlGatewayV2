      
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

#include "base_fun.h"
#include "sqlite3.h"
#include "database.h"

extern INT32 set_lamp_module_state(UINT8 lamp_module_id, UINT16 state);
extern void websocket_statechange_send_msg(const INT8 *p_msg);

#define CREATE_LAMP_TABLE "create table if not exists lamp(" \
	"module_id tinyint not null, " \
	"bit_id tinyint not null, " \
	"state int not null, " \
	"capa tinyint not null, " \
	"primary key(module_id, bit_id));"

#define CREATE_LAMP_INFO_TABLE "create table if not exists lamp_info(" \
	"lamp_module_id tinyint not null, " \
	"lamp_bit_id tinyint not null, " \
	"lamp_name nvarchar(200), " \
	"primary key(lamp_module_id, lamp_bit_id));"
	
#define CREATE_KEY_TABLE "create table if not exists key(" \
	"module_id tinyint not null, " \
	"bit_id tinyint not null, " \
	"capa tinyint not null, " \
	"primary key(module_id, bit_id));"

#define CREATE_KEY_BIND_TABLE "create table if not exists key_bind(" \
	"key_module_id tinyint not null, " \
	"key_bit_id tinyint not null, " \
	"key_name nvarchar(200), " \
	"lamp_module_id tinyint, " \
	"lamp_bit_id tinyint, " \
	"scene_number tinyint, " \
	"primary key(key_module_id, key_bit_id));"

#define CREATE_SCENE_TABLE "create table if not exists scene(" \
	"number tinyint not null primary key, " \
	"trigger_time int not null, " \
	"name nvarchar(200) ," \
	"`date` text ," \
	"`wday` text " \
	");"
	
#define CREATE_SCENE_LAMP_TABLE "create table if not exists scene_lamp(" \
	"scene_number tinyint not null, " \
	"lamp_module_id tinyint not null, " \
	"lamp_bit_id tinyint not null, " \
	"lamp_state int not null, " \
	"primary key(scene_number, lamp_module_id, lamp_bit_id));"

static sqlite3 *sqlite_db;

static INT32 database_patch(void)
{
	INT8 *err_msg = 0;
	sqlite3_stmt *stmt;
	INT32 ret = 0;
	INT32 scene_column_count = 0;

	ret = sqlite3_prepare_v2(sqlite_db, "select * from scene;", -1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	scene_column_count = sqlite3_column_count(stmt);

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (scene_column_count <= 3) // 老版本数据库中的scene表，需要修改
	{
		DEBUG_PRINT(DEBUG_NOTICE, "old database scene table\n");
		
		if (sqlite3_exec(sqlite_db, "alter table scene add `date` text;", NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			return ERROR;
		}

		if (sqlite3_exec(sqlite_db, "alter table scene add `wday` text;", NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			return ERROR;
		}
	}

	return OK;
}

/**		  
 * @brief sqlite数据库初始化(连接数据库，创建数据表)
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 database_init(void)
{
	INT8 *err_msg = 0;
	
	if (sqlite3_open("lampcontrol.db", &sqlite_db) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_open database failed:%s\n", sqlite3_errmsg(sqlite_db));
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, CREATE_LAMP_TABLE, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, CREATE_LAMP_INFO_TABLE, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, CREATE_KEY_TABLE, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, CREATE_KEY_BIND_TABLE, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, CREATE_SCENE_TABLE, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, CREATE_SCENE_LAMP_TABLE, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(sqlite_db);
		return ERROR;
	}

	if (database_patch() != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "database_patch failed\n");
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 关闭sqlite数据库
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 database_close(void)
{
	INT32 ret = 0;

	ret = sqlite3_close(sqlite_db);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_close failed:ret=%d\n", ret);
		return ERROR;
	}
	else
	{
		return OK;
	}
}

/**		  
 * @brief 开始一个事务
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 begin_transaction(void)
{
	INT8 *err_msg = 0;

	if (sqlite3_exec(sqlite_db, "begin;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 提交一个事务
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 commit_transaction(void)
{
	INT8 *err_msg = 0;

	if (sqlite3_exec(sqlite_db, "commit;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 回滚一个事务
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 rollback_transaction(void)
{
	INT8 *err_msg = 0;

	if (sqlite3_exec(sqlite_db, "rollback;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 清除lamp和key两张表中的数据
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 clear_device_table(void)
{
	INT8 *err_msg = 0;

	if (sqlite3_exec(sqlite_db, "delete from lamp;delete from key;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

INT32 add_lamp_module(INT32 module_id, INT32 io_num, INT32 state)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};
	INT32 i = 0;
	INT32 lamp_state = 0;

	begin_transaction();

	for (i = 0; i < io_num; i++)
	{
		if (((1 << i) & state) == 0)
		{
			lamp_state = 0;
		}
		else
		{
			lamp_state = 1;
		}
		
		snprintf(sql_str, sizeof(sql_str), "insert into lamp values(%d,%d,%d,%d);"
			, module_id, i, lamp_state, io_num);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			rollback_transaction();
			return ERROR;
		}
	}

	commit_transaction();

	return OK;
}

/**		  
 * @brief 设备增加或变更为lamp(将原module_id对应的设备在lamp或key表中删除，然后在lamp表中增加新设备)
 * @param[in] module_id IO模块拨码地址
 * @param[in] io_num IO口数量
 * @param[in] state 按位表示的IO口状态
 * @return OK-成功，ERROR-失败
 */
INT32 change_device_to_lamp(INT32 module_id, INT32 io_num, INT32 state)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str), "delete from lamp where module_id=%d;", module_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	snprintf(sql_str, sizeof(sql_str), "delete from key where module_id=%d;", module_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	if (add_lamp_module(module_id, io_num, state) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "add_lamp_module failed\n");
		return ERROR;
	}

	return OK;
}

INT32 add_key_module(INT32 module_id, INT32 key_num)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};
	INT32 i = 0;

	begin_transaction();
	for (i = 0; i < key_num; i++)
	{
		snprintf(sql_str, sizeof(sql_str), "insert into key values(%d,%d,%d);"
			, module_id, i, key_num);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			rollback_transaction();
			return ERROR;
		}
	}
	commit_transaction();

	return OK;
}

/**		  
 * @brief 设备增加或变更为key(将原module_id对应的设备在lamp或key表中删除，然后在key表中增加新设备)
 * @param[in] module_id 按键模块拨码地址
 * @param[in] key_num 按键数量
 * @return OK-成功，ERROR-失败
 */
INT32 change_device_to_key(INT32 module_id, INT32 key_num)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str), "delete from lamp where module_id=%d;", module_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	snprintf(sql_str, sizeof(sql_str), "delete from key where module_id=%d;", module_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	if (add_key_module(module_id, key_num) != OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "add_key_module failed\n");
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 删除module_id对应的设备(在lamp或key表中删除module_id对应的设备)
 * @param[in] module_id IO模块或按键模块拨码地址
 * @return OK-成功，ERROR-失败
 */
INT32 delete_device(UINT8 module_id)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str), "delete from lamp where module_id=%u;", module_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	snprintf(sql_str, sizeof(sql_str), "delete from key where module_id=%u;", module_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 删除场景信息,删除按键绑定信息
 * @param 无
 * @return OK-成功，ERROR-失败
 */
INT32 clear_bind_and_scene(void)
{
	INT8 *err_msg = 0;

	begin_transaction();
	
	if (sqlite3_exec(sqlite_db, "delete from scene_lamp;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		rollback_transaction();
		return ERROR;
	}
	
	if (sqlite3_exec(sqlite_db, "delete from scene;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		rollback_transaction();
		return ERROR;
	}

	if (sqlite3_exec(sqlite_db, "update key_bind set lamp_module_id=null,lamp_bit_id=null,scene_number=null;", NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		rollback_transaction();
		return ERROR;
	}
	
	commit_transaction();

	return OK;
}

/**		  
 * @brief 获取单个灯的状态切换，关->开 或 开->关
 * @param[in] lamp_module_id IO模块拨码地址
 * @param[in] lamp_bit_id IO模块位地址
 * @return OK-成功，ERROR-失败
 */
INT32 trigger_lamp(UINT8 lamp_module_id, INT32 lamp_bit_id)
{
	UINT16 module_state = 0;

	if (get_lamp_module_state(lamp_module_id, &module_state) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "get_lamp_module_state failed\n");
		return ERROR;
	}
	
	module_state = module_state ^ (1 << lamp_bit_id);
	
	set_lamp_module_state(lamp_module_id, module_state);
	
	return OK;
}

/**		  
 * @brief 设置单个灯的状态
 * @param[in] lamp_module_id IO模块拨码地址
 * @param[in] lamp_bit_id IO模块位地址
 * @param[in] state 灯状态
 * @return OK-成功，ERROR-失败
 */
INT32 set_lamp_state(UINT8 lamp_module_id, INT32 lamp_bit_id, INT32 state)
{
	UINT16 module_state = 0;

	if (get_lamp_module_state(lamp_module_id, &module_state) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "get_lamp_module_state failed\n");
		return ERROR;
	}
	
	if (0 == state)
	{
		module_state = module_state & ~((UINT16)(1 << lamp_bit_id));
	}
	else
	{
		module_state = module_state | (1 << lamp_bit_id);
	}
	
	set_lamp_module_state(lamp_module_id, module_state);
	
	return OK;
}

INT32 update_module_state_by_scene(INT32 scene_number, UINT8 module_id, UINT16 *p_state)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 ret = 0;
	INT32 len = 0;
	
	len = snprintf(sql_str, sizeof(sql_str), 
				   "select lamp_bit_id,lamp_state from scene_lamp where scene_number=%d and lamp_module_id=%u"
				   , scene_number, module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret)
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			if (sqlite3_column_int(stmt, 1) == 0)
			{
				*p_state &= ~((UINT16)(1 << sqlite3_column_int(stmt, 0)));
			}
			else
			{
				*p_state |= (1 << sqlite3_column_int(stmt, 0));
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			break;
		}
	}
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}
	return OK;
}

INT32 get_lamp_module_state(UINT8 module_id, UINT16 *p_state)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 ret = 0;
	INT32 len = 0;
	INT32 bit_id = 0;
	UINT16 state = 0;
	
	len = snprintf(sql_str, sizeof(sql_str), 
				   "select state,bit_id from lamp where module_id=%u;"
				   , module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret)
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			bit_id = sqlite3_column_int(stmt, 1);
			if (0 == sqlite3_column_int(stmt, 0))
			{
				state &= ~((UINT16)(1 << bit_id));
			}
			else
			{
				state |= (UINT16)(1 << bit_id);
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			sqlite3_finalize(stmt);
			return ERROR;
		}
	}
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}
	
	*p_state = state;
	return OK;
}

INT32 get_lamp_module(LAMP_MODULE *p_lamp_module, INT32 module_num)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 ret = 0;
	INT32 len = 0;
	INT32 i = 0;

	len = snprintf(sql_str, sizeof(sql_str), 
				   "select module_id from lamp group by module_id;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	while (i < module_num)
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret)
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{	
			p_lamp_module[i].id = (UINT8)sqlite3_column_int(stmt, 0);
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			break;
		}
		i++;
	}
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}
	
	module_num = i;
	for (i = 0; i < module_num; i++)
	{
		get_lamp_module_state(p_lamp_module[i].id, &p_lamp_module[i].state);
	}
	
	return module_num;
}

INT32 get_scene_lamp_module(INT32 scene_number, LAMP_MODULE *p_lamp_module, INT32 module_num)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 ret = 0;
	INT32 len = 0;
	INT32 i = 0;

	len = snprintf(sql_str, sizeof(sql_str), 
				   "select lamp_module_id from scene_lamp where scene_number=%d group by lamp_module_id;"
				   , scene_number);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	while (i < module_num)
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret)
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{	
			p_lamp_module[i].id = (UINT8)sqlite3_column_int(stmt, 0);
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			break;
		}
		i++;
	}
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}
	
	module_num = i;
	for (i = 0; i < module_num; i++)
	{
		get_lamp_module_state(p_lamp_module[i].id, &p_lamp_module[i].state);
		update_module_state_by_scene(scene_number, p_lamp_module[i].id, &p_lamp_module[i].state);
	}
	
	return module_num;
}

/**		  
 * @brief 触发场景号为number的场景
 * @param[in] number 场景号
 * @return OK-成功，ERROR-失败
 */
INT32 trigger_scene_by_module(INT32 scene_number)
{
	LAMP_MODULE lamp_module[MAX_LAMP_MODULE];
	INT32 module_num = 0;
	INT32 i = 0;
	
	module_num = get_scene_lamp_module(scene_number, lamp_module, MAX_LAMP_MODULE);
	if (module_num < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "get_scene_lamp_module error\n");
		return ERROR;
	}
	
	for (i = 0; i < module_num; i++)
	{
		set_lamp_module_state(lamp_module[i].id, lamp_module[i].state);
	}
	
	return OK;
}

/**		  
 * @brief 设置所有灯的状态
 * @param[in] on TRUE-全开，FALSE-全关
 * @return OK-成功，ERROR-失败
 */
INT32 set_all_lamp_module_state(BOOL on)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[128] = {0};
	INT32 ret = 0;
	INT32 len = 0;
	UINT8 lamp_module_id = 0;

	len = snprintf(sql_str, sizeof(sql_str), 
				   "select module_id from lamp group by module_id;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret)
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{	
			lamp_module_id = (UINT8)sqlite3_column_int(stmt, 0);
			if (on)
			{
				set_lamp_module_state(lamp_module_id, 0xFFFF);
			}
			else
			{
				set_lamp_module_state(lamp_module_id, 0x0);
			}
			
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			break;
		}
	}
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}
	
	return OK;
}

/**		  
 * @brief 模块状态变化更新(IO模块变化表示灯的开关状态变化，按键模块变化表示有按键被按下)
 * @param[in] module_id IO模块或按键模块拨码地址
 * @param[in] state 按位表示的状态
 * @return OK-成功，ERROR-失败
 */
INT32 change_device_state(INT32 module_id, INT32 state)
{
	sqlite3_stmt *stmt;
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 number = 0;
	INT32 i = 0;
	INT32 lamp_state = 0;
	INT32 key_bit_id = -1;
	UINT8 lamp_module_id = 0;
	INT32 lamp_bit_id = -1;
	INT32 scene_number = -1;

	/* 在lamp表中查找module_id确定是否是IO模块状态发生变化 */
	len = snprintf(sql_str, sizeof(sql_str), "select capa from lamp where module_id=%d;", module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据，设备为lamp
	{
		number = sqlite3_column_int(stmt, 0);
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in lamp table: number=0x%02X\n", number);
	}
	else if (SQLITE_DONE == ret) // 没有查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find module_id in lamp table!\n");
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (number > 0) // IO模块状态变化，更新lamp状态
	{
		begin_transaction();
		for (i = 0; i < number; i++)
		{
			if (((1 << i) & state) == 0)
			{
				lamp_state = 0;
			}
			else
			{
				lamp_state = 1;
			}
			
			snprintf(sql_str, sizeof(sql_str)
				, "update lamp set state=%d where(module_id=%d and bit_id=%d);"
				, lamp_state, module_id, i);
			if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
			{
				DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
				sqlite3_free(err_msg);
				rollback_transaction();
				return ERROR;
			}
		}
		commit_transaction();

		websocket_statechange_send_msg("light");
		return OK;
	}

	/* 处理按键模块按下后对应的状态变化 */
	/* 查找key表中按键是否存在并获取其能力 */
	number = 0;
	len = snprintf(sql_str, sizeof(sql_str), "select capa from key where module_id=%d;", module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据，按键存在
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in key table!\n");
		number = sqlite3_column_int(stmt, 0);
	}
	else if (SQLITE_DONE == ret) // 没有查找到数据，按键不存在
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find module_id in key table!\n");
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (0 == number) // module_id既没有对应的IO模块也没有对应的按键模块
	{
		DEBUG_PRINT(DEBUG_WARN, "there is no device has module_id=0x%02X!\n", module_id);
		return OK;
	}

	/* 查找被按下的那个按键，低位键优先 */
	for (i = 0; i < number; i++)
	{
		if (((1 << i) & state) != 0)
		{
			key_bit_id = i;
			break;
		}
	}
	if (-1 == key_bit_id)
	{
		DEBUG_PRINT(DEBUG_WARN, "no key has been pressed!\n");
		return OK;
	}
	DEBUG_PRINT(DEBUG_NOTICE
		, "the key(key_module_id=0x%02X key_bit_id=%d capa=%d) has been pressed!\n"
		, module_id, key_bit_id, number);

	/* 查找被按下的按键所绑定的场景或灯 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select lamp_module_id,lamp_bit_id,scene_number from key_bind "
		"where(key_module_id=%d and key_bit_id=%d);"
		, module_id, key_bit_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据
	{
		if (SQLITE_NULL != sqlite3_column_type(stmt, 0) && SQLITE_NULL != sqlite3_column_type(stmt, 1)) // 按键绑定灯
		{
			DEBUG_PRINT(DEBUG_NOTICE, "the key is bind a lamp!\n");
			lamp_module_id = (UINT8)(sqlite3_column_int(stmt, 0) & 0xFF);
			lamp_bit_id = sqlite3_column_int(stmt, 1);
		}
		else if (SQLITE_NULL != sqlite3_column_type(stmt, 2)) // 按键绑定场景
		{
			DEBUG_PRINT(DEBUG_NOTICE, "the key is bind a scene!\n");
			scene_number = sqlite3_column_int(stmt, 2);
		}
		else
		{
			DEBUG_PRINT(DEBUG_NOTICE, "the key is not binded!\n");
		}
	}
	else if (SQLITE_DONE == ret) // 按键未绑定
	{
		DEBUG_PRINT(DEBUG_NOTICE, "the key is not binded!\n");
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (lamp_bit_id != -1) //  灯状态切换
	{
		return trigger_lamp(lamp_module_id, lamp_bit_id);
	}
	else if (scene_number != -1) // 触发场景生效
	{
		return trigger_scene_by_module(scene_number);
	}
	else
	{
		return OK;
	}
}

/**		  
 * @brief 获取灯列表，通过JSON格式数据返回
 * @param[out] p_buff 用于返回JSON数据的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return OK-成功，ERROR-失败
 */
INT32 get_lamp_list(INT8 *p_buff, INT32 buff_len)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 lamp_state = 0;
	INT32 lamp_capa = 0;
	INT8 lamp_name[210] = {0};
	INT8 type_str[8] ={0};
	INT8 state_str[8] ={0};
	INT32 id = 0;
	INT32 module_id = 0;
	INT32 bit_id = 0;

	if (NULL == p_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	/* 在lamp表和lamp_info中查找所有灯的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select module_id,bit_id,state,capa,lamp_name from lamp left join lamp_info "
		"on(module_id=lamp_module_id and bit_id=lamp_bit_id) "
		"order by module_id asc, bit_id asc;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	len = snprintf(p_buff, buff_len, "%s", "[");

	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret) // 已经找到所有灯
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			module_id = sqlite3_column_int(stmt, 0);
			bit_id = sqlite3_column_int(stmt, 1);
			id = (module_id << 8) | bit_id;
			
			lamp_state = sqlite3_column_int(stmt, 2);
			if (0 == lamp_state)
			{
				strcpy(state_str, "false");
			}
			else
			{
				strcpy(state_str, "true");
			}
			
			lamp_capa = sqlite3_column_int(stmt, 3);
			if (4 == lamp_capa)
			{
				strcpy(type_str, "4IO");
			}
			else if (8 == lamp_capa)
			{
				strcpy(type_str, "8IO");
			}
			else if (12 == lamp_capa)
			{
				strcpy(type_str, "12IO");
			}
			else
			{
				strcpy(type_str, "UNKNOWN");
			}
			
			if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
			{
				strcpy(lamp_name, (INT8 *)sqlite3_column_text(stmt, 4));
			}
			else
			{
				memset(lamp_name, 0, sizeof(lamp_name));
			}

			len += snprintf(p_buff + len, buff_len - len, "{\"id\":%d,\"type\":\"%s\",\"on\":%s,\"name\":\"%s\"},"
				, id, type_str, state_str, lamp_name);
			if (len > buff_len - 1)
			{
				DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
				(void)sqlite3_finalize(stmt);
				return ERROR;
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (1 == len) // 灯列表为空
	{
		len++;
	}
	
	p_buff[len - 1] = ']'; // 将最后','改为']'
	p_buff[len] = '\0';
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_lamp_list:\n%s\n", p_buff);

	return OK;
}

/**		  
 * @brief 获取单个灯的信息，通过JSON格式数据返回
 * @param[in] lamp_module_id IO模块拨码地址
 * @param[in] lamp_bit_id IO模块位地址
 * @param[out] p_buff 用于返回JSON数据的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return OK-成功，ERROR-失败
 */
INT32 get_lamp_info(UINT8 lamp_module_id, INT32 lamp_bit_id, LAMP_INFO *p_lamp_info)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;

	if (NULL == p_lamp_info)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	/* 在lamp表和lamp_info中查找单个灯的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select state,capa,lamp_name from lamp left join lamp_info "
		"on(module_id=lamp_module_id and bit_id=lamp_bit_id) "
		"where(module_id=%u and bit_id=%d);"
		, lamp_module_id, lamp_bit_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据，设备为lamp
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in lamp table!\n");
		p_lamp_info->state = sqlite3_column_int(stmt, 0);
		p_lamp_info->capa = sqlite3_column_int(stmt, 1);
		if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
		{
			strcpy(p_lamp_info->name, (INT8 *)sqlite3_column_text(stmt, 2));
		}
	}
	else if (SQLITE_DONE == ret) // 没有查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find lamp!\n");
		return ERROR;
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
		return ERROR;
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	return OK;
}

/**		  
 * @brief 设置灯的名称
 * @param[in] lamp_module_id IO模块拨码地址
 * @param[in] lamp_bit_id IO模块位地址
 * @param[in] p_lamp_name 需要设置的灯的名称
 * @return OK-成功，ERROR-失败
 */
INT32 set_lamp_name(UINT8 lamp_module_id, INT32 lamp_bit_id, const INT8 *p_lamp_name)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 op = 0;
	INT8 *err_msg = 0;

	if (NULL == p_lamp_name)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}
	
	/* 在lamp表和lamp_info中查找单个灯的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select lamp_name from lamp left join lamp_info "
		"on(module_id=lamp_module_id and bit_id=lamp_bit_id) "
		"where(module_id=%u and bit_id=%d);"
		, lamp_module_id, lamp_bit_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据，设备为lamp
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in lamp table!\n");
		if (sqlite3_column_type(stmt, 0) == SQLITE_NULL)
		{
			op = 1; // lamp_info中没有该灯名称，需要插入一个新的记录
		}
		else
		{
			op = 2; // lamp_info已经存在该灯名称，需要更新一个记录
		}
	}
	else if (SQLITE_DONE == ret) // 没有查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find lamp!\n");
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (1 == op)
	{
		snprintf(sql_str, sizeof(sql_str), "insert into lamp_info values(%u,%d,'%s');"
			, lamp_module_id, lamp_bit_id, p_lamp_name);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			return ERROR;
		}
	}
	else if (2 == op)
	{
		snprintf(sql_str, sizeof(sql_str)
			, "update lamp_info set lamp_name='%s' where(lamp_module_id=%u and lamp_bit_id=%d);"
			, p_lamp_name, lamp_module_id, lamp_bit_id);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			return ERROR;
		}
	}
	else
	{
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 设置数据库中的灯的开关状态
 * @param[in] lamp_module_id IO模块拨码地址
 * @param[in] lamp_bit_id IO模块位地址
 * @param[in] state 0-关 1-开
 * @return 成功返回该lamp_module_id对应的所有IO的状态信息，失败返回ERROR
 */
INT32 set_lamp_state_to_db(UINT8 lamp_module_id, INT32 lamp_bit_id, INT32 state)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 op = 0;
	INT32 lamp_state = 0;
	INT32 ret_state = 0;
	INT32 bit_id = 0;
	INT8 *err_msg = 0;
	
	/* 在lamp表中查找单个灯的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select bit_id,state from lamp where(module_id=%u);", lamp_module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret) // 已经找到所有灯
		{
			break;
		}
		else if (SQLITE_ROW == ret) // 查找到数据
		{
			bit_id = sqlite3_column_int(stmt, 0);
			lamp_state = sqlite3_column_int(stmt, 1);
			
			if (bit_id == lamp_bit_id)
			{
				DEBUG_PRINT(DEBUG_NOTICE, "find lamp_bit_id in lamp table!\n");
				op = 1; // lamp中找到对应的灯，需要更新一个记录
				lamp_state = state;
			}

			if (0 == lamp_state)
			{
				ret_state &= ~(1 << bit_id);
			}
			else
			{
				ret_state |= (1 << bit_id);
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (1 == op)
	{
		snprintf(sql_str, sizeof(sql_str)
			, "update lamp set state=%d where(module_id=%u and bit_id=%d);"
			, state, lamp_module_id, lamp_bit_id);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			return ERROR;
		}
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "can not find lamp!\n");
		return ERROR;
	}

	return ret_state;
}

INT32 set_lamp_module_state_to_db(UINT8 lamp_module_id, INT32 state)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 lamp_state = 0;
	INT32 bit_id = 0;
	INT8 *err_msg = 0;
	INT32 capa = 0;
	
	/* 在lamp表中查找单个灯的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
				   "select capa from lamp where(module_id=%u);", lamp_module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	
	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据
	{
		capa = sqlite3_column_int(stmt, 0);
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
		(void)sqlite3_finalize(stmt);
		return ERROR;
	}
	(void)sqlite3_finalize(stmt);
	
	/* 开始事务 */
	begin_transaction();

	for (bit_id = 0; bit_id < capa; bit_id++)
	{
		if (((1 << bit_id) & state) == 0)
		{
			lamp_state = 0;
		}
		else
		{
			lamp_state = 1;
		}
		snprintf(sql_str, sizeof(sql_str)
				 , "update lamp set state=%d where(module_id=%u and bit_id=%d);"
				 , lamp_state, lamp_module_id, bit_id);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			rollback_transaction();
			return ERROR;
		}
	}

	/* 提交事务 */
	commit_transaction();

	return OK;
}

/**		  
 * @brief 创建新场景
 * @param[out] p_scene_info 返回新增场景的信息
 * @return OK-成功，ERROR-失败
 */
INT32 create_new_scene(SCENE_INFO *p_scene_info)
{
	sqlite3_stmt *stmt;
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 new_number = 1;
	INT32 number = 0;

	if (NULL == p_scene_info)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}
	
	/* 在scene表中查找所有已存在的场景并按场景号排序 */
	len = snprintf(sql_str, sizeof(sql_str)
		, "select number from scene order by number asc;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret) // 已经遍历所有场景
		{
			break;
		}
		else if (SQLITE_ROW == ret) // 查找到场景
		{
			number = sqlite3_column_int(stmt, 0);
			if (new_number != number)
			{
				break;
			}
			else
			{
				new_number++;
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	/* 创建新场景 */
	snprintf(sql_str, sizeof(sql_str), "insert into scene values(%d,%d,'%s','%s','%s');"
		, new_number, -1, "", "", "");
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	memset(p_scene_info, 0, sizeof(SCENE_INFO));
	p_scene_info->number = new_number;
	p_scene_info->trigger_time = -1;

	return OK;
}


/**		  
 * @brief 查找是否存在对应number的场景，不存在则创建新场景
 * @param[in] number 场景号
 * @param[out] p_trigger_time 返回场景的触发时间
 * @param[out] p_name 返回场景名称
 * @param[in] name_len 场景名称p_name缓冲区长度
 * @return OK-成功，ERROR-失败
 */
INT32 find_and_create_scene(INT32 number, INT32 *p_time, INT8 *p_name, INT32 name_len, 
	INT8 *p_date, INT32 date_len, INT8 *p_wday, INT32 wday_len)
{
	sqlite3_stmt *stmt;
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 op = 0;

	if (NULL == p_time || NULL == p_name || NULL == p_date || NULL == p_wday || name_len < 1)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}
	
	/* 在scene表中查找对应number的场景是否存在 */
	len = snprintf(sql_str, sizeof(sql_str)
		, "select trigger_time,name,`date`,wday from scene where number=%d;", number);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find number in scene table!\n");
		*p_time = sqlite3_column_int(stmt, 0);
		snprintf(p_name, name_len, "%s", (INT8 *)sqlite3_column_text(stmt, 1));
		if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
		{
			snprintf(p_date, date_len, "%s", (INT8 *)sqlite3_column_text(stmt, 2));
		}
		else
		{
			memset(p_date, 0, date_len);
		}
		if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
		{
			snprintf(p_wday, wday_len, "%s", (INT8 *)sqlite3_column_text(stmt, 3));
		}
		else
		{
			memset(p_wday, 0, wday_len);
		}
	}
	else if (SQLITE_DONE == ret) // 没有查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find number in scene table!\n");
		op = 1; // 需要创建新的场景
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
		(void)sqlite3_finalize(stmt);
		return ERROR;
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (1 == op)
	{
		*p_time = -1;
		strcpy(p_name, "");
		strcpy(p_date, "");
		strcpy(p_wday, "");
		
		snprintf(sql_str, sizeof(sql_str), "insert into scene values(%d,%d,'%s','%s','%s');"
			, number, *p_time, p_name, p_date, p_wday);
		if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
			sqlite3_free(err_msg);
			return ERROR;
		}
	}

	return OK;
}

/**		  
 * @brief 更新scene表中的场景参数
 * @param[in] number 场景号
 * @param[in] trigger_time 场景的触发时间
 * @param[in] p_name 场景名称
 * @return OK-成功，ERROR-失败
 */
INT32 update_scene_param(INT32 number, INT32 trigger_time, const INT8 *p_name, const INT8 *p_date, const INT8 *p_wday)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str)
		, "update scene set trigger_time=%d,name='%s',`date`='%s',wday='%s' where number=%d;"
		, trigger_time, p_name, p_date, p_wday, number);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 删除scene_lamp表中的场景号为number对应的所有灯
 * @param[in] number 场景号
 * @return OK-成功，ERROR-失败
 */
INT32 delete_scene_lamp(INT32 number)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str), "delete from scene_lamp where scene_number=%d;", number);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief scene_lamp表中插入场景号为number对应的一个灯
 * @param[in] number 场景号
 * @param[in] lamp_module_id 灯模块id
 * @param[in] lamp_bit_id 灯位id
 * @param[in] lamp_state 灯状态
 * @return OK-成功，ERROR-失败
 */
INT32 add_scene_lamp(INT32 number, UINT8 lamp_module_id, INT32 lamp_bit_id, INT32 lamp_state)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str), "insert into scene_lamp values(%d,%u,%d,%d);"
		, number, lamp_module_id, lamp_bit_id, lamp_state);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

/**		  
 * @brief 获取场景列表，通过JSON格式数据返回
 * @param[out] p_buff 用于返回JSON数据的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return OK-成功，ERROR-失败
 */
INT32 get_scene_list(INT8 *p_buff, INT32 buff_len)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 lamp_state = 0;
	INT8 state_str[8] ={0};
	INT32 number = 0;
	INT32 last_number = -1;
	INT8 scene_name[210] = {0};
	INT32 time = -1;
	INT8 date[16] = {0};
	INT8 wday[16] = {0};
	INT8 time_str[8] = {0};
	INT8 trigger_time[64] = {0};
	INT32 lamp_id = 0;
	INT32 lamp_module_id = 0;
	INT32 lamp_bit_id = 0;

	if (NULL == p_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	/* 在scene表和scene_lamp中查找所有场景的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select number,name,trigger_time,lamp_module_id,lamp_bit_id,lamp_state,`date`,wday "
		"from scene left join scene_lamp on number=scene_number "
		"order by number asc, lamp_module_id asc, lamp_bit_id asc;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	len = snprintf(p_buff, buff_len, "%s", "[");

	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret) // 已经找到所有场景
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			number = sqlite3_column_int(stmt, 0);
			snprintf(scene_name, sizeof(scene_name), "%s", (INT8 *)sqlite3_column_text(stmt, 1));
			if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
			{
				snprintf(date, sizeof(date), "%s", (INT8 *)sqlite3_column_text(stmt, 6));
			}
			else
			{
				memset(date, 0, sizeof(date));
			}
			if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
			{
				snprintf(wday, sizeof(wday), "%s", (INT8 *)sqlite3_column_text(stmt, 7));
			}
			else
			{
				memset(wday, 0, sizeof(wday));
			}
			time = sqlite3_column_int(stmt, 2);
			if (-1 == time)
			{
				strcpy(time_str, "");
			}
			else
			{
				snprintf(time_str, sizeof(time_str), "%02d:%02d"
					, (time >> 8) & 0xFF, time & 0xFF);
			}
			snprintf(trigger_time, sizeof(trigger_time), "{\"date\":\"%s\",\"wday\":[%s],\"time\":\"%s\"}", 
				date, wday, time_str);

			if (last_number != number)
			{
				if (last_number != -1) // 第一条记录不需要加灯列表的结束字符串
				{
					if (',' == p_buff[len - 1])
					{
						len--;
					}
					len += snprintf(p_buff + len, buff_len - len, "]},");
					if (len >= buff_len)
					{
						DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
						(void)sqlite3_finalize(stmt);
						return ERROR;
					}
				}
				
				len += snprintf(p_buff + len, buff_len - len
					, "{\"id\":%d,\"name\":\"%s\",\"trigger_time\":%s,\"lights\":["
					, number, scene_name, trigger_time);
				if (len >= buff_len)
				{
					DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
					(void)sqlite3_finalize(stmt);
					return ERROR;
				}
			}
			
			if (sqlite3_column_type(stmt, 3) != SQLITE_NULL 
				&& sqlite3_column_type(stmt, 4) != SQLITE_NULL)
			{
				lamp_module_id = sqlite3_column_int(stmt, 3);
				lamp_bit_id = sqlite3_column_int(stmt, 4);
				lamp_id = (lamp_module_id << 8) | lamp_bit_id;
				lamp_state = sqlite3_column_int(stmt, 5);
				if (0 == lamp_state)
				{
					strcpy(state_str, "false");
				}
				else
				{
					strcpy(state_str, "true");
				}
				len += snprintf(p_buff + len, buff_len - len, "{\"id\":%d,\"on\":%s},"
					, lamp_id, state_str);
				if (len >= buff_len)
				{
					DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
					(void)sqlite3_finalize(stmt);
					return ERROR;
				}
			}

			last_number = number;
			
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (1 == len) // 场景列表为空
	{
		len += snprintf(p_buff + len, buff_len - len, "]");
	}
	else
	{
		if (',' == p_buff[len - 1])
		{
			len--;
		}
		len += snprintf(p_buff + len, buff_len - len, "]}]");
	}
	
	if (len >= buff_len)
	{
		DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
		return ERROR;
	}
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_scene_list:\n%s\n", p_buff);

	return OK;
}

/**		  
 * @brief 获取单个场景的信息，通过JSON格式数据返回
 * @param[in] number 场景号
 * @param[out] p_buff 用于返回JSON数据的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return OK-成功，ERROR-失败
 */
INT32 get_scene_info(INT32 number, INT8 *p_buff, INT32 buff_len)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 lamp_state = 0;
	INT32 lamp_id = 0;
	INT32 lamp_module_id = 0;
	INT32 lamp_bit_id = 0;
	INT8 scene_name[210] = {0};
	INT32 time = -1;
	INT8 date[16] = {0};
	INT8 wday[16] = {0};
	INT8 time_str[8] = {0};
	INT8 trigger_time[64] = {0};
	INT8 state_str[8] ={0};
	BOOL has_scene = FALSE;

	if (NULL == p_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	/* 在scene表和scene_lamp中查找单个场景的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select name,trigger_time,lamp_module_id,lamp_bit_id,lamp_state,`date`,wday "
		"from scene left join scene_lamp on number=scene_number "
		"where number=%d "
		"order by lamp_module_id asc, lamp_bit_id asc;"
		, number);

	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	len = 0;
	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret)
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			snprintf(scene_name, sizeof(scene_name), "%s", (INT8 *)sqlite3_column_text(stmt, 0));
			if (sqlite3_column_type(stmt, 5) != SQLITE_NULL)
			{
				snprintf(date, sizeof(date), "%s", (INT8 *)sqlite3_column_text(stmt, 5));
			}
			else
			{
				memset(date, 0, sizeof(date));
			}
			if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
			{
				snprintf(wday, sizeof(wday), "%s", (INT8 *)sqlite3_column_text(stmt, 6));
			}
			else
			{
				memset(wday, 0, sizeof(wday));
			}
			time = sqlite3_column_int(stmt, 1);
			if (-1 == time)
			{
				strcpy(time_str, "");
			}
			else
			{
				snprintf(time_str, sizeof(time_str), "%02d:%02d"
					, (time >> 8) & 0xFF, time & 0xFF);
			}
			snprintf(trigger_time, sizeof(trigger_time), "{\"date\":\"%s\",\"wday\":[%s],\"time\":\"%s\"}", 
				date, wday, time_str);

			if (!has_scene)
			{
				len += snprintf(p_buff + len, buff_len - len
					, "{\"id\":%d,\"name\":\"%s\",\"trigger_time\":%s,\"lights\":["
					, number, scene_name, trigger_time);
				if (len >= buff_len)
				{
					DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
					(void)sqlite3_finalize(stmt);
					return ERROR;
				}
			}
			
			if (sqlite3_column_type(stmt, 2) != SQLITE_NULL 
				&& sqlite3_column_type(stmt, 3) != SQLITE_NULL)
			{
				lamp_module_id = sqlite3_column_int(stmt, 2);
				lamp_bit_id = sqlite3_column_int(stmt, 3);
				lamp_id = (lamp_module_id << 8) | lamp_bit_id;
				lamp_state = sqlite3_column_int(stmt, 4);
				if (0 == lamp_state)
				{
					strcpy(state_str, "false");
				}
				else
				{
					strcpy(state_str, "true");
				}
				len += snprintf(p_buff + len, buff_len - len, "{\"id\":%d,\"on\":%s},"
					, lamp_id, state_str);
				if (len >= buff_len)
				{
					DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
					(void)sqlite3_finalize(stmt);
					return ERROR;
				}
			}

			has_scene = TRUE;
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (!has_scene)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find scene!\n");
		return ERROR;
	}
	else
	{
		if (',' == p_buff[len - 1])
		{
			len--;
		}
		len += snprintf(p_buff + len, buff_len - len, "]}");
	}
	
	if (len >= buff_len)
	{
		DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
		return ERROR;
	}
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_scene_info:\n%s\n", p_buff);

	return OK;

}


/**		  
 * @brief 删除场景号为number场景信息
 * @param[in] number 场景号
 * @return OK-成功，ERROR-失败
 */
INT32 delete_scene_info(INT32 number)
{
	INT8 *err_msg = 0;
	INT8 sql_str[128] = {0};

	snprintf(sql_str, sizeof(sql_str), "delete from scene where number=%d;", number);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return delete_scene_lamp(number);
}

/**		  
 * @brief 获取场景的定时触发信息
 * @param[in]  scene_array 定时触发场景数组
 * @return OK-成功，ERROR-失败
 */
INT32 get_scene_time_info(TIME_SCENE *scene_array)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[64] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 number = 0;
	
	/* 在scene表中查找所有场景的信息 */
	len = snprintf(sql_str, sizeof(sql_str), "select number,trigger_time,`date`,wday from scene;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret) // 已经找到所有场景
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			number = sqlite3_column_int(stmt, 0);
			if (number > MAX_SCENE_NUM)
			{
				DEBUG_PRINT(DEBUG_WARN, "get_scene_time_info exceed max scene number(number=%d)\n", number);
				continue;
			}

			scene_array[number - 1].exsit = TRUE;
			scene_array[number - 1].trigger_time = sqlite3_column_int(stmt, 1);
			if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
			{
				snprintf(scene_array[number - 1].trigger_date, sizeof(scene_array[number - 1].trigger_date), "%s", 
						 (INT8 *)sqlite3_column_text(stmt, 2));
			}
			else
			{
				memset(scene_array[number - 1].trigger_date, 0, sizeof(scene_array[number - 1].trigger_date));
			}
			if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
			{
				snprintf(scene_array[number - 1].trigger_wday, sizeof(scene_array[number - 1].trigger_wday), "%s", 
						 (INT8 *)sqlite3_column_text(stmt, 3));
			}
			else
			{
				memset(scene_array[number - 1].trigger_wday, 0, sizeof(scene_array[number - 1].trigger_wday));
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	return OK;
}


/**		  
 * @brief 获取按键列表，通过JSON格式数据返回
 * @param[out] p_buff 用于返回JSON数据的缓冲区
 * @param[in]  buff_len 缓冲区长度
 * @return OK-成功，ERROR-失败
 */
INT32 get_key_list(INT8 *p_buff, INT32 buff_len)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 key_capa = 0;
	INT8 key_name[210] = {0};
	INT8 type_str[8] ={0};
	INT32 id = 0;
	INT32 module_id = 0;
	INT32 bit_id = 0;
	INT32 lamp_module_id = 0;
	INT32 lamp_bit_id = 0;
	INT8 bind_info[48] = {0};

	if (NULL == p_buff || buff_len <= 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	/* 在key表和key_bind中查找所有按键的信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select module_id,bit_id,capa,key_name,lamp_module_id,lamp_bit_id,scene_number "
		"from key left join key_bind on(module_id=key_module_id and bit_id=key_bit_id) "
		"order by module_id asc, bit_id asc;");
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	len = snprintf(p_buff, buff_len, "%s", "[");

	FOREVER
	{
		ret = sqlite3_step(stmt);
		if (SQLITE_DONE == ret) // 已经找到所有按键
		{
			break;
		}
		else if (SQLITE_ROW == ret)
		{
			module_id = sqlite3_column_int(stmt, 0);
			bit_id = sqlite3_column_int(stmt, 1);
			id = (module_id << 8) | bit_id;
			
			key_capa = sqlite3_column_int(stmt, 2);
			if (4 == key_capa)
			{
				strcpy(type_str, "4KEY");
			}
			else if (6 == key_capa)
			{
				strcpy(type_str, "6KEY");
			}
			else if (8 == key_capa)
			{
				strcpy(type_str, "8KEY");
			}
			else
			{
				strcpy(type_str, "UNKNOWN");
			}
			
			if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
			{
				strcpy(key_name, (INT8 *)sqlite3_column_text(stmt, 3));
			}
			else
			{
				memset(key_name, 0, sizeof(key_name));
			}

			if (sqlite3_column_type(stmt, 4) != SQLITE_NULL 
				&& sqlite3_column_type(stmt, 5) != SQLITE_NULL) // 按键绑定的是灯
			{
				lamp_module_id = sqlite3_column_int(stmt, 4);
				lamp_bit_id = sqlite3_column_int(stmt, 5);
				snprintf(bind_info, sizeof(bind_info)
					, "{\"type\":\"light\",\"id\":%d}", (lamp_module_id << 8) | lamp_bit_id);
			}
			else if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) // 按键绑定的是场景
			{
				snprintf(bind_info, sizeof(bind_info)
					, "{\"type\":\"scene\",\"id\":%d}", sqlite3_column_int(stmt, 6));
			}
			else // 按键未绑定
			{
				strcpy(bind_info, "{}");
			}

			len += snprintf(p_buff + len, buff_len - len
				, "{\"id\":%d,\"type\":\"%s\",\"name\":\"%s\",\"bind\":%s},"
				, id, type_str, key_name, bind_info);
			if (len > buff_len - 1)
			{
				DEBUG_PRINT(DEBUG_ERROR, "p_buff do not have enough space!\n");
				(void)sqlite3_finalize(stmt);
				return ERROR;
			}
		}
		else
		{
			DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
			(void)sqlite3_finalize(stmt);
			return ERROR;
		}
	}

	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	if (1 == len) // 按键列表为空
	{
		len++;
	}
	
	p_buff[len - 1] = ']'; // 将最后','改为']'
	p_buff[len] = '\0';
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_key_list:\n%s\n", p_buff);

	return OK;
}

/**		  
 * @brief 获取单个按键的相关信息
 * @param[in] key_module_id 按键模块拨码地址
 * @param[in] key_bit_id 按键模块位地址
 * @param[out] p_key_info 用于返回获取的信息
 * @return OK-成功，ERROR-失败
 */
INT32 get_key_info(UINT8 key_module_id, INT32 key_bit_id, KEY_INFO *p_key_info)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[256] = {0};
	INT32 len = 0;
	INT32 ret = 0;

	if (NULL == p_key_info)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	/* 在key表和key_bind表中查找按键绑定信息 */
	len = snprintf(sql_str, sizeof(sql_str), 
		"select capa,key_name,lamp_module_id,lamp_bit_id,scene_number "
		"from key left join key_bind on(module_id=key_module_id and bit_id=key_bit_id) "
		"where(module_id=%u and bit_id=%d);"
		, key_module_id, key_bit_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // 查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find info in key and key_bind table!\n");

		p_key_info->capa = sqlite3_column_int(stmt, 0);
		
		if (sqlite3_column_type(stmt, 1) != SQLITE_NULL)
		{
			strcpy(p_key_info->name, (INT8 *)sqlite3_column_text(stmt, 1));
		}
		else
		{
			memset(p_key_info->name, 0, sizeof(p_key_info->name));
		}
			
		if (sqlite3_column_type(stmt, 2) != SQLITE_NULL 
			&& sqlite3_column_type(stmt, 3) != SQLITE_NULL) // 按键绑定的是灯
		{
			p_key_info->bind_type = BIND_LAMP;
			p_key_info->lamp_module_id = sqlite3_column_int(stmt, 2);
			p_key_info->lamp_bit_id = sqlite3_column_int(stmt, 3);
		}
		else if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) // 按键绑定的是场景
		{
			p_key_info->bind_type = BIND_SCENE;
			p_key_info->scene_number = sqlite3_column_int(stmt, 4);
		}
		else // 按键未绑定
		{
			p_key_info->bind_type = BIND_NONE;
		}
	}
	else if (SQLITE_DONE == ret) // 没有查找到数据
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find key info in key and key_bind table!\n");
		p_key_info->capa = 0;
		memset(p_key_info->name, 0, sizeof(p_key_info->name));
		p_key_info->bind_type = BIND_NONE;
		(void)sqlite3_finalize(stmt);
		return ERROR;
	}
	else
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_step failed! error code=%d\n", ret);
		(void)sqlite3_finalize(stmt);
		return ERROR;
	}
	
	ret = sqlite3_finalize(stmt);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_finalize failed! error code=%d\n", ret);
	}

	return OK;
}


/**		  
 * @brief 设置按键的相关信息
 * @param[in] key_module_id 按键模块拨码地址
 * @param[in] key_bit_id 按键模块位地址
 * @param[in] p_key_info 需要设置的按键相关信息
 * @return OK-成功，ERROR-失败
 */
INT32 set_key_info(UINT8 key_module_id, INT32 key_bit_id, const KEY_INFO *p_key_info)
{
	INT8 *err_msg = 0;
	INT8 sql_str[256] = {0};

	/* 删除原有信息 */
	snprintf(sql_str, sizeof(sql_str)
		, "delete from key_bind where(key_module_id=%u and key_bit_id=%d);"
		, key_module_id, key_bit_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	/* 增加新信息 */
	if (BIND_LAMP == p_key_info->bind_type)
	{
		snprintf(sql_str, sizeof(sql_str), 
			"insert into key_bind(key_module_id,key_bit_id,key_name,lamp_module_id,lamp_bit_id) "
			"values(%d,%d,'%s',%d,%d);"
			, key_module_id, key_bit_id, p_key_info->name, p_key_info->lamp_module_id, p_key_info->lamp_bit_id);
	}
	else if (BIND_SCENE == p_key_info->bind_type)
	{
		snprintf(sql_str, sizeof(sql_str), 
			"insert into key_bind(key_module_id,key_bit_id,key_name,scene_number) "
			"values(%d,%d,'%s',%d);"
			, key_module_id, key_bit_id, p_key_info->name, p_key_info->scene_number);
	}
	else
	{
		snprintf(sql_str, sizeof(sql_str), 
			"insert into key_bind(key_module_id,key_bit_id,key_name) "
			"values(%d,%d,'%s');"
			, key_module_id, key_bit_id, p_key_info->name);
	}
	
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	return OK;
}

