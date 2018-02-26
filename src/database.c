      
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

	if (scene_column_count <= 3) // �ϰ汾���ݿ��е�scene����Ҫ�޸�
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
 * @brief sqlite���ݿ��ʼ��(�������ݿ⣬�������ݱ�)
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief �ر�sqlite���ݿ�
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ��ʼһ������
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief �ύһ������
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief �ع�һ������
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ���lamp��key���ű��е�����
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief �豸���ӻ���Ϊlamp(��ԭmodule_id��Ӧ���豸��lamp��key����ɾ����Ȼ����lamp�����������豸)
 * @param[in] module_id IOģ�鲦���ַ
 * @param[in] io_num IO������
 * @param[in] state ��λ��ʾ��IO��״̬
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief �豸���ӻ���Ϊkey(��ԭmodule_id��Ӧ���豸��lamp��key����ɾ����Ȼ����key�����������豸)
 * @param[in] module_id ����ģ�鲦���ַ
 * @param[in] key_num ��������
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ɾ��module_id��Ӧ���豸(��lamp��key����ɾ��module_id��Ӧ���豸)
 * @param[in] module_id IOģ��򰴼�ģ�鲦���ַ
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ɾ��������Ϣ,ɾ����������Ϣ
 * @param ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ��ȡ�����Ƶ�״̬�л�����->�� �� ��->��
 * @param[in] lamp_module_id IOģ�鲦���ַ
 * @param[in] lamp_bit_id IOģ��λ��ַ
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ���õ����Ƶ�״̬
 * @param[in] lamp_module_id IOģ�鲦���ַ
 * @param[in] lamp_bit_id IOģ��λ��ַ
 * @param[in] state ��״̬
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ����������Ϊnumber�ĳ���
 * @param[in] number ������
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief �������еƵ�״̬
 * @param[in] on TRUE-ȫ����FALSE-ȫ��
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ģ��״̬�仯����(IOģ��仯��ʾ�ƵĿ���״̬�仯������ģ��仯��ʾ�а���������)
 * @param[in] module_id IOģ��򰴼�ģ�鲦���ַ
 * @param[in] state ��λ��ʾ��״̬
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��lamp���в���module_idȷ���Ƿ���IOģ��״̬�����仯 */
	len = snprintf(sql_str, sizeof(sql_str), "select capa from lamp where module_id=%d;", module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // ���ҵ����ݣ��豸Ϊlamp
	{
		number = sqlite3_column_int(stmt, 0);
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in lamp table: number=0x%02X\n", number);
	}
	else if (SQLITE_DONE == ret) // û�в��ҵ�����
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

	if (number > 0) // IOģ��״̬�仯������lamp״̬
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

	/* ������ģ�鰴�º��Ӧ��״̬�仯 */
	/* ����key���а����Ƿ���ڲ���ȡ������ */
	number = 0;
	len = snprintf(sql_str, sizeof(sql_str), "select capa from key where module_id=%d;", module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // ���ҵ����ݣ���������
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in key table!\n");
		number = sqlite3_column_int(stmt, 0);
	}
	else if (SQLITE_DONE == ret) // û�в��ҵ����ݣ�����������
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

	if (0 == number) // module_id��û�ж�Ӧ��IOģ��Ҳû�ж�Ӧ�İ���ģ��
	{
		DEBUG_PRINT(DEBUG_WARN, "there is no device has module_id=0x%02X!\n", module_id);
		return OK;
	}

	/* ���ұ����µ��Ǹ���������λ������ */
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

	/* ���ұ����µİ������󶨵ĳ������ */
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
	if (SQLITE_ROW == ret) // ���ҵ�����
	{
		if (SQLITE_NULL != sqlite3_column_type(stmt, 0) && SQLITE_NULL != sqlite3_column_type(stmt, 1)) // �����󶨵�
		{
			DEBUG_PRINT(DEBUG_NOTICE, "the key is bind a lamp!\n");
			lamp_module_id = (UINT8)(sqlite3_column_int(stmt, 0) & 0xFF);
			lamp_bit_id = sqlite3_column_int(stmt, 1);
		}
		else if (SQLITE_NULL != sqlite3_column_type(stmt, 2)) // �����󶨳���
		{
			DEBUG_PRINT(DEBUG_NOTICE, "the key is bind a scene!\n");
			scene_number = sqlite3_column_int(stmt, 2);
		}
		else
		{
			DEBUG_PRINT(DEBUG_NOTICE, "the key is not binded!\n");
		}
	}
	else if (SQLITE_DONE == ret) // ����δ��
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

	if (lamp_bit_id != -1) //  ��״̬�л�
	{
		return trigger_lamp(lamp_module_id, lamp_bit_id);
	}
	else if (scene_number != -1) // ����������Ч
	{
		return trigger_scene_by_module(scene_number);
	}
	else
	{
		return OK;
	}
}

/**		  
 * @brief ��ȡ���б�ͨ��JSON��ʽ���ݷ���
 * @param[out] p_buff ���ڷ���JSON���ݵĻ�����
 * @param[in]  buff_len ����������
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��lamp���lamp_info�в������еƵ���Ϣ */
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
		if (SQLITE_DONE == ret) // �Ѿ��ҵ����е�
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

	if (1 == len) // ���б�Ϊ��
	{
		len++;
	}
	
	p_buff[len - 1] = ']'; // �����','��Ϊ']'
	p_buff[len] = '\0';
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_lamp_list:\n%s\n", p_buff);

	return OK;
}

/**		  
 * @brief ��ȡ�����Ƶ���Ϣ��ͨ��JSON��ʽ���ݷ���
 * @param[in] lamp_module_id IOģ�鲦���ַ
 * @param[in] lamp_bit_id IOģ��λ��ַ
 * @param[out] p_buff ���ڷ���JSON���ݵĻ�����
 * @param[in]  buff_len ����������
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��lamp���lamp_info�в��ҵ����Ƶ���Ϣ */
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
	if (SQLITE_ROW == ret) // ���ҵ����ݣ��豸Ϊlamp
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in lamp table!\n");
		p_lamp_info->state = sqlite3_column_int(stmt, 0);
		p_lamp_info->capa = sqlite3_column_int(stmt, 1);
		if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
		{
			strcpy(p_lamp_info->name, (INT8 *)sqlite3_column_text(stmt, 2));
		}
	}
	else if (SQLITE_DONE == ret) // û�в��ҵ�����
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
 * @brief ���õƵ�����
 * @param[in] lamp_module_id IOģ�鲦���ַ
 * @param[in] lamp_bit_id IOģ��λ��ַ
 * @param[in] p_lamp_name ��Ҫ���õĵƵ�����
 * @return OK-�ɹ���ERROR-ʧ��
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
	
	/* ��lamp���lamp_info�в��ҵ����Ƶ���Ϣ */
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
	if (SQLITE_ROW == ret) // ���ҵ����ݣ��豸Ϊlamp
	{
		DEBUG_PRINT(DEBUG_NOTICE, "find module_id in lamp table!\n");
		if (sqlite3_column_type(stmt, 0) == SQLITE_NULL)
		{
			op = 1; // lamp_info��û�иõ����ƣ���Ҫ����һ���µļ�¼
		}
		else
		{
			op = 2; // lamp_info�Ѿ����ڸõ����ƣ���Ҫ����һ����¼
		}
	}
	else if (SQLITE_DONE == ret) // û�в��ҵ�����
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
 * @brief �������ݿ��еĵƵĿ���״̬
 * @param[in] lamp_module_id IOģ�鲦���ַ
 * @param[in] lamp_bit_id IOģ��λ��ַ
 * @param[in] state 0-�� 1-��
 * @return �ɹ����ظ�lamp_module_id��Ӧ������IO��״̬��Ϣ��ʧ�ܷ���ERROR
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
	
	/* ��lamp���в��ҵ����Ƶ���Ϣ */
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
		if (SQLITE_DONE == ret) // �Ѿ��ҵ����е�
		{
			break;
		}
		else if (SQLITE_ROW == ret) // ���ҵ�����
		{
			bit_id = sqlite3_column_int(stmt, 0);
			lamp_state = sqlite3_column_int(stmt, 1);
			
			if (bit_id == lamp_bit_id)
			{
				DEBUG_PRINT(DEBUG_NOTICE, "find lamp_bit_id in lamp table!\n");
				op = 1; // lamp���ҵ���Ӧ�ĵƣ���Ҫ����һ����¼
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
	
	/* ��lamp���в��ҵ����Ƶ���Ϣ */
	len = snprintf(sql_str, sizeof(sql_str), 
				   "select capa from lamp where(module_id=%u);", lamp_module_id);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}
	
	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // ���ҵ�����
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
	
	/* ��ʼ���� */
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

	/* �ύ���� */
	commit_transaction();

	return OK;
}

/**		  
 * @brief �����³���
 * @param[out] p_scene_info ����������������Ϣ
 * @return OK-�ɹ���ERROR-ʧ��
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
	
	/* ��scene���в��������Ѵ��ڵĳ����������������� */
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
		if (SQLITE_DONE == ret) // �Ѿ��������г���
		{
			break;
		}
		else if (SQLITE_ROW == ret) // ���ҵ�����
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

	/* �����³��� */
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
 * @brief �����Ƿ���ڶ�Ӧnumber�ĳ������������򴴽��³���
 * @param[in] number ������
 * @param[out] p_trigger_time ���س����Ĵ���ʱ��
 * @param[out] p_name ���س�������
 * @param[in] name_len ��������p_name����������
 * @return OK-�ɹ���ERROR-ʧ��
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
	
	/* ��scene���в��Ҷ�Ӧnumber�ĳ����Ƿ���� */
	len = snprintf(sql_str, sizeof(sql_str)
		, "select trigger_time,name,`date`,wday from scene where number=%d;", number);
	ret = sqlite3_prepare_v2(sqlite_db, sql_str, len + 1, &stmt, NULL);
	if (ret != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_prepare_v2 failed! error code=%d\n", ret);
		return ERROR;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW == ret) // ���ҵ�����
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
	else if (SQLITE_DONE == ret) // û�в��ҵ�����
	{
		DEBUG_PRINT(DEBUG_NOTICE, "can not find number in scene table!\n");
		op = 1; // ��Ҫ�����µĳ���
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
 * @brief ����scene���еĳ�������
 * @param[in] number ������
 * @param[in] trigger_time �����Ĵ���ʱ��
 * @param[in] p_name ��������
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ɾ��scene_lamp���еĳ�����Ϊnumber��Ӧ�����е�
 * @param[in] number ������
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief scene_lamp���в��볡����Ϊnumber��Ӧ��һ����
 * @param[in] number ������
 * @param[in] lamp_module_id ��ģ��id
 * @param[in] lamp_bit_id ��λid
 * @param[in] lamp_state ��״̬
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ��ȡ�����б�ͨ��JSON��ʽ���ݷ���
 * @param[out] p_buff ���ڷ���JSON���ݵĻ�����
 * @param[in]  buff_len ����������
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��scene���scene_lamp�в������г�������Ϣ */
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
		if (SQLITE_DONE == ret) // �Ѿ��ҵ����г���
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
				if (last_number != -1) // ��һ����¼����Ҫ�ӵ��б�Ľ����ַ���
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

	if (1 == len) // �����б�Ϊ��
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
 * @brief ��ȡ������������Ϣ��ͨ��JSON��ʽ���ݷ���
 * @param[in] number ������
 * @param[out] p_buff ���ڷ���JSON���ݵĻ�����
 * @param[in]  buff_len ����������
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��scene���scene_lamp�в��ҵ�����������Ϣ */
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
 * @brief ɾ��������Ϊnumber������Ϣ
 * @param[in] number ������
 * @return OK-�ɹ���ERROR-ʧ��
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
 * @brief ��ȡ�����Ķ�ʱ������Ϣ
 * @param[in]  scene_array ��ʱ������������
 * @return OK-�ɹ���ERROR-ʧ��
 */
INT32 get_scene_time_info(TIME_SCENE *scene_array)
{
	sqlite3_stmt *stmt;
	INT8 sql_str[64] = {0};
	INT32 len = 0;
	INT32 ret = 0;
	INT32 number = 0;
	
	/* ��scene���в������г�������Ϣ */
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
		if (SQLITE_DONE == ret) // �Ѿ��ҵ����г���
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
 * @brief ��ȡ�����б�ͨ��JSON��ʽ���ݷ���
 * @param[out] p_buff ���ڷ���JSON���ݵĻ�����
 * @param[in]  buff_len ����������
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��key���key_bind�в������а�������Ϣ */
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
		if (SQLITE_DONE == ret) // �Ѿ��ҵ����а���
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
				&& sqlite3_column_type(stmt, 5) != SQLITE_NULL) // �����󶨵��ǵ�
			{
				lamp_module_id = sqlite3_column_int(stmt, 4);
				lamp_bit_id = sqlite3_column_int(stmt, 5);
				snprintf(bind_info, sizeof(bind_info)
					, "{\"type\":\"light\",\"id\":%d}", (lamp_module_id << 8) | lamp_bit_id);
			}
			else if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) // �����󶨵��ǳ���
			{
				snprintf(bind_info, sizeof(bind_info)
					, "{\"type\":\"scene\",\"id\":%d}", sqlite3_column_int(stmt, 6));
			}
			else // ����δ��
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

	if (1 == len) // �����б�Ϊ��
	{
		len++;
	}
	
	p_buff[len - 1] = ']'; // �����','��Ϊ']'
	p_buff[len] = '\0';
	
	DEBUG_PRINT(DEBUG_NOTICE, "get_key_list:\n%s\n", p_buff);

	return OK;
}

/**		  
 * @brief ��ȡ���������������Ϣ
 * @param[in] key_module_id ����ģ�鲦���ַ
 * @param[in] key_bit_id ����ģ��λ��ַ
 * @param[out] p_key_info ���ڷ��ػ�ȡ����Ϣ
 * @return OK-�ɹ���ERROR-ʧ��
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

	/* ��key���key_bind���в��Ұ�������Ϣ */
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
	if (SQLITE_ROW == ret) // ���ҵ�����
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
			&& sqlite3_column_type(stmt, 3) != SQLITE_NULL) // �����󶨵��ǵ�
		{
			p_key_info->bind_type = BIND_LAMP;
			p_key_info->lamp_module_id = sqlite3_column_int(stmt, 2);
			p_key_info->lamp_bit_id = sqlite3_column_int(stmt, 3);
		}
		else if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) // �����󶨵��ǳ���
		{
			p_key_info->bind_type = BIND_SCENE;
			p_key_info->scene_number = sqlite3_column_int(stmt, 4);
		}
		else // ����δ��
		{
			p_key_info->bind_type = BIND_NONE;
		}
	}
	else if (SQLITE_DONE == ret) // û�в��ҵ�����
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
 * @brief ���ð����������Ϣ
 * @param[in] key_module_id ����ģ�鲦���ַ
 * @param[in] key_bit_id ����ģ��λ��ַ
 * @param[in] p_key_info ��Ҫ���õİ��������Ϣ
 * @return OK-�ɹ���ERROR-ʧ��
 */
INT32 set_key_info(UINT8 key_module_id, INT32 key_bit_id, const KEY_INFO *p_key_info)
{
	INT8 *err_msg = 0;
	INT8 sql_str[256] = {0};

	/* ɾ��ԭ����Ϣ */
	snprintf(sql_str, sizeof(sql_str)
		, "delete from key_bind where(key_module_id=%u and key_bit_id=%d);"
		, key_module_id, key_bit_id);
	if (sqlite3_exec(sqlite_db, sql_str, NULL, NULL, &err_msg) != SQLITE_OK)
	{
		DEBUG_PRINT(DEBUG_ERROR, "sqlite3_exec error:%s\n", err_msg);
		sqlite3_free(err_msg);
		return ERROR;
	}

	/* ��������Ϣ */
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

