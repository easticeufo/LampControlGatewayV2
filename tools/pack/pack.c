
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

/**		  
 * @brief		打包程序主函数入口
 * @param[in] argc 命令行中参数的个数
 * @param[in] argv 命令行参数数组
 * @return OK 
 */
INT32 main(INT32 argc, INT8 *argv[])
{
	INT8 *p_firmware_name = NULL;
	INT32 firm_fd = 0;
	INT32 file_fd = 0;
	UPGRADE_FILE_HEADER *p_file_header = NULL;
	FIRMWARE_HEADER *p_firm_header = NULL;
	INT32 file_num = 0;
	INT32 file_offset = 0;
	INT32 header_len = 0;
	INT32 file_len = 0;
	UINT8 *p_file_buff = NULL;
	INT32 i = 0;

	if (argc < 2)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_firmware_name = argv[1];
	/* 创建升级包文件 */
	firm_fd = open(p_firmware_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (ERROR == firm_fd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "open file failed:%s\n", strerror(errno));
		return ERROR;
	}

	file_num = argc - 2;

	header_len = sizeof(FIRMWARE_HEADER) + file_num * sizeof(UPGRADE_FILE_HEADER);
	p_firm_header = (FIRMWARE_HEADER *)malloc(header_len);
	if (NULL == p_firm_header)
	{
		DEBUG_PRINT(DEBUG_ERROR, "malloc failed\n");
		SAFE_CLOSE(firm_fd);
		return ERROR;
	}
	memset(p_firm_header, 0, header_len);
	p_file_header = (UPGRADE_FILE_HEADER *)(p_firm_header + 1);

	/* 写入打包文件 */
	file_offset = header_len;
	lseek(firm_fd, header_len, SEEK_SET);
	for (i = 0; i < file_num; i++)
	{
		/* 打开打包文件 */
		snprintf(p_file_header[i].file_name, sizeof(p_file_header[i].file_name), "%s", argv[i+2]);
		file_fd = open(p_file_header[i].file_name, O_RDONLY);
		if (file_fd == ERROR)
		{
			DEBUG_PRINT(DEBUG_ERROR, "open file failed:%s\n", strerror(errno));
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_firm_header);
			return ERROR;
		}

		file_len = lseek(file_fd, 0, SEEK_END);
		p_file_buff = (UINT8 *)malloc(file_len);
		if (NULL == p_file_buff)
		{
			DEBUG_PRINT(DEBUG_ERROR, "malloc failed\n");
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_firm_header);
			SAFE_CLOSE(file_fd);
			return ERROR;
		}
		memset(p_file_buff, 0, file_len);
		lseek(file_fd, 0, SEEK_SET);
		if (readn(file_fd, p_file_buff, file_len) != file_len)
		{
			DEBUG_PRINT(DEBUG_ERROR, "readn failed\n");
			SAFE_CLOSE(firm_fd);
			SAFE_FREE(p_firm_header);
			SAFE_CLOSE(file_fd);
			SAFE_FREE(p_file_buff);
			return ERROR;
		}
		writen(firm_fd, p_file_buff, file_len);

		p_file_header[i].check_sum = checksum_u8(p_file_buff, file_len);
		p_file_header[i].file_len = file_len;
		p_file_header[i].start_offset = file_offset;

		SAFE_CLOSE(file_fd);
		SAFE_FREE(p_file_buff);
		file_offset += file_len;
	}

	/* 写入头部信息 */
	p_firm_header->magic_number = PACK_MAGIC_NUMBER;
	p_firm_header->file_num = file_num;
	lseek(firm_fd, 0, SEEK_SET);
	writen(firm_fd, p_firm_header, header_len);
	SAFE_CLOSE(firm_fd);
	SAFE_FREE(p_firm_header);

	printf("pack success!\n");
	
	return OK;
}

