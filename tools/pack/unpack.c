
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author  madongfang
 * @date     2016-7-6
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "pack.h"

#define UNPACK_DIR "unpack_file" // 解包后的文件存放目录

/**		  
 * @brief		解包程序主函数入口
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	INT8 *p_firmware_name = NULL;
	INT32 firm_fd = 0;
	INT32 file_fd = 0;
	INT8 file_path_name[64] = {0};
	UPGRADE_FILE_HEADER *p_file_header = NULL;
	FIRMWARE_HEADER firm_header;
	INT32 file_num = 0;
	UINT8 *p_file_buff = NULL;
	INT32 i = 0;

	if (argc < 2)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_firmware_name = argv[1];
	/* 打开升级包文件 */
	firm_fd = open(p_firmware_name, O_RDONLY);
	if (ERROR == firm_fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open %s failed:%s\n", p_firmware_name, strerror(errno));
		return ERROR;
	}

	memset(&firm_header, 0, sizeof(FIRMWARE_HEADER));
	if (readn(firm_fd, &firm_header, sizeof(FIRMWARE_HEADER)) != sizeof(FIRMWARE_HEADER))
	{
		DEBUG_PRINT(DEBUG_ERROR, "readn failed\n");
		SAFE_CLOSE(firm_fd);
		return ERROR;
	}

	file_num = firm_header.file_num;
	if (firm_header.magic_number != PACK_MAGIC_NUMBER || file_num < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR
			, "magic number or file number error! magic_number=0x%x file_num=%d\n"
			, firm_header.magic_number, file_num);
		SAFE_CLOSE(firm_fd);
		return ERROR;
	}

	p_file_header = (UPGRADE_FILE_HEADER *)malloc(file_num * sizeof(UPGRADE_FILE_HEADER));
	if (NULL == p_file_header)
	{
		DEBUG_PRINT(DEBUG_ERROR, "malloc failed\n");
		SAFE_CLOSE(firm_fd);
		return ERROR;
	}

	/* 读取firmware头部文件信息 */
	memset(p_file_header, 0, file_num * sizeof(UPGRADE_FILE_HEADER));
	if (readn(firm_fd, p_file_header, file_num * sizeof(UPGRADE_FILE_HEADER)) 
		!= file_num * sizeof(UPGRADE_FILE_HEADER))
	{
		DEBUG_PRINT(DEBUG_ERROR, "readn failed\n");
		SAFE_CLOSE(firm_fd);
		SAFE_FREE(p_file_header);
		return ERROR;
	}

	if (mkdir(UNPACK_DIR, 0777) == ERROR)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mkdir %s failed:%s\n", UNPACK_DIR, strerror(errno));
		SAFE_CLOSE(firm_fd);
		SAFE_FREE(p_file_header);
		return ERROR;
	}

	/* 读取并校验各个文件内容，生成解压后的文件 */
	for (i = 0; i < file_num; i++)
	{
		p_file_buff = (UINT8 *)malloc(p_file_header[i].file_len);
		if (NULL == p_file_buff)
		{
			DEBUG_PRINT(DEBUG_ERROR, "malloc failed\n");
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_file_header);
			return ERROR;
		}
		
		memset(p_file_buff, 0, p_file_header[i].file_len);
		lseek(firm_fd, p_file_header[i].start_offset, SEEK_SET);
		if (readn(firm_fd, p_file_buff, p_file_header[i].file_len) != p_file_header[i].file_len)
		{
			DEBUG_PRINT(DEBUG_ERROR, "readn failed\n");
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_file_header);
			SAFE_FREE(p_file_buff);
			return ERROR;
		}

		if (checksum_u8(p_file_buff, p_file_header[i].file_len) != p_file_header[i].check_sum)
		{
			DEBUG_PRINT(DEBUG_ERROR, "checksum_u8 failed\n");
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_file_header);
			SAFE_FREE(p_file_buff);
			return ERROR;
		}
		
		snprintf(file_path_name, sizeof(file_path_name), "%s/%s", UNPACK_DIR, p_file_header[i].file_name);
		file_fd = open(file_path_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (ERROR == file_fd)
		{
			DEBUG_PRINT(DEBUG_ERROR, "open %s failed:%s\n", file_path_name, strerror(errno));
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_file_header);
			SAFE_FREE(p_file_buff);
			return ERROR;
		}

		writen(file_fd, p_file_buff, p_file_header[i].file_len);
		SAFE_CLOSE(file_fd);
		SAFE_FREE(p_file_buff);
	}

	SAFE_FREE(p_file_header);
	SAFE_CLOSE(firm_fd);

	printf("unpack success!\n");
	
	return OK;
}

