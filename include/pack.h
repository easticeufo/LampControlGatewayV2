      
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

#define PACK_MAGIC_NUMBER 0x19871023 // ��ʼħ����

/**		  
 * @brief firmware�и����ļ�����Ϣͷ������FIRMWARE_HEADER֮������Ϊfirmware�а������ļ���
 */
typedef struct
{
	INT8 file_name[32]; // �ļ���
	INT32 start_offset; // ��ʼλ��
	INT32 file_len; // �ļ�����
	UINT32 check_sum; // �����
	UINT8 res[32];
}UPGRADE_FILE_HEADER;

/**		  
 * @brief firmwareͷ����Ϣ������firmware�ʼ��
 */
typedef struct
{
	UINT32 magic_number; // ��ʼħ����
	INT32 file_num; // �ļ�����
	UINT8 res[32];
}FIRMWARE_HEADER;

#endif

