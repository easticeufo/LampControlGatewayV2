      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-7-6
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#ifndef _PACK_H_
#define _PACK_H_

#define PACK_MAGIC_NUMBER 0x19871023 // 起始魔术字

/**		  
 * @brief firmware中各个文件的信息头，放在FIRMWARE_HEADER之后，数量为firmware中包含的文件数
 */
typedef struct
{
	INT8 file_name[32]; // 文件名
	INT32 start_offset; // 起始位置
	INT32 file_len; // 文件长度
	UINT32 check_sum; // 检验和
	UINT8 res[32];
}UPGRADE_FILE_HEADER;

/**		  
 * @brief firmware头部信息，放在firmware最开始处
 */
typedef struct
{
	UINT32 magic_number; // 起始魔术字
	INT32 file_num; // 文件个数
	UINT8 res[32];
}FIRMWARE_HEADER;

#endif

