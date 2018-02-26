      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author  madongfang
 * @date     2016-5-26
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"

static INT32 debug_level = DEBUG_ERROR; // ���Դ�ӡ��Ϣ����ȼ�

/**		  
 * @brief ��ȡ���Դ�ӡ��Ϣ����ȼ�
 * @param ��
 * @return ���Դ�ӡ��Ϣ����ȼ�
 */
INT32 get_debug_level(void)
{
	return debug_level;
}

/**		  
 * @brief		���õ��Դ�ӡ��Ϣ����ȼ� 
 * @param[in] level ���Դ�ӡ��Ϣ����ȼ�
 * @return ��
 */
void set_debug_level(INT32 level)
{
	debug_level = level;
}

/**
 * @brief ��ȡ����������ڣ�����һ��������ʾ������,��p_date_buff����NULL����p_date_buff�з��ر������ڵ��ַ���
 * @param[out] p_date_buff �����ڰ��ո�ʽ"build yyyymmdd"���
 * @param[in] buff_len ��ű��������ַ����Ļ���������
 * @return ����һ��������ʾ����,16~31λ��ʾ���,8~15λ��ʾ�·�,0~7λ��ʾ����
 */
UINT32 get_build_date(INT8 *p_date_buff, INT32 buff_len)
{
	INT32 year = 0;
	INT32 month = 0;
	INT32 day = 0;
	INT8 month_name[4] = {0};
	const INT8 *all_mon_names[] 
		= {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	sscanf(__DATE__, "%s%d%d", month_name, &day, &year);

	for (month = 0; month < 12; month++)
	{
		if (strcmp(month_name, all_mon_names[month]) == 0)
		{
			break;
		}
	}
	month++;

	if (p_date_buff != NULL)
	{
		snprintf(p_date_buff, buff_len, "build %d%02d%02d", year, month, day);
	}

	return (UINT32)((year << 16) | (month << 8) | day);
}

/**		  
 * @brief		�����̺߳�����װ  
 * @param[in] p_func �̺߳���ָ��
 * @param[in] arg ���ݸ��̻߳ص������Ĳ���
 * @return	�ɹ�����OK��ʧ�ܷ���ERROR
 */
INT32 thread_create(void *(* p_fun)(void *), void *arg)
{
	pthread_t tid = 0;
	pthread_attr_t attr;
	
	if (OK != pthread_attr_init(&attr))
	{
		DEBUG_PRINT(DEBUG_ERROR, "pthread_attr_init failed\n");
		return ERROR;
	}
	
	/* ���÷����߳����� */
	if (OK != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
	{
		DEBUG_PRINT(DEBUG_ERROR, "pthread_attr_setdetachstate failed\n");
		pthread_attr_destroy(&attr);
		return ERROR;
	}
	
	if (OK != pthread_create(&tid, &attr, p_fun, arg))
	{
		DEBUG_PRINT(DEBUG_ERROR, "pthread_create failed\n");
		pthread_attr_destroy(&attr);
		return ERROR;
	}
	pthread_attr_destroy(&attr);
	return OK;
}

/**		  
 * @brief		 ��UINT8��ʽ���㻺���������ݵ�У���
 * @param[in]  p_data ���ݻ�����
 * @param[in]  nums ���ݳ���
 * @return	 У���,����������󷵻�ERROR(0xffffffff)
 */
UINT32 checksum_u8(const void *p_data, INT32 nums)
{
	UINT32 checksum = 0;
	const UINT8 *p_buf = NULL;
	INT32 i = 0;

	if (NULL == p_data || nums < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	p_buf = (const UINT8 *)p_data;
	for (i = 0; i < nums; i++)
	{
		checksum += *p_buf;
		p_buf++;
	}
	return checksum;
}

/**		  
 * @brief		 ��ָ�����ȵ����ݣ�����ʱ�����ӶϿ�������ļ�β�򷵻أ���������
 * @param[in]  nums �������ܳ���
 * @param[in]  fd �򿪵�������
 * @param[out] p_buf �����ݻ�����
 * @return	 ������ERROR�����򷵻�ʵ�ʶ��������ݳ���
 */
INT32 readn(INT32 fd, void *p_buf, INT32 nums)
{
	INT32 nleft = nums;
	INT32 nread = 0;
	UINT8 *ptr = (UINT8 *)p_buf;
	INT32 ret = 0;
	fd_set rset;
	struct timeval timeout;

	if (fd < 0 || NULL == p_buf || nums < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	while (nleft > 0)
	{
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		timeout.tv_sec = 4;
		timeout.tv_usec = 0;
		ret = select(fd + 1, &rset, NULL, NULL, &timeout);

		if (ret < 0)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_INFO, "select: %s\n", strerror(errno));
				continue;
			}
			else
			{
				DEBUG_PRINT(DEBUG_WARN, "select failed: %s\n", strerror(errno));
				return ERROR;
			}
		}
		else if (0 == ret)
		{
			DEBUG_PRINT(DEBUG_WARN, "select timeout\n");
			break;
		}
		else
		{
			if ((nread = read(fd, ptr, nleft)) < 0)
			{
				if (EINTR == errno)
				{
					DEBUG_PRINT(DEBUG_INFO, "read: %s\n", strerror(errno));
					nread = 0;
				}
				else
				{
					DEBUG_PRINT(DEBUG_ERROR, "read failed: %s\n", strerror(errno));
					return ERROR;
				}
			}
			else if (0 == nread)
			{
				/* ���ӹرջ�����ļ�β */
				break;
			}
			else
			{
				nleft -= nread;
				ptr += nread;
			}
		}
	}
	
	if (nleft > 0)
	{
		DEBUG_PRINT(DEBUG_WARN, "expect read %d bytes,actually read %d bytes\n", nums, (nums - nleft));
	}
	return (nums - nleft);
}

/**		  
 * @brief		 ����дָ�����ȵ�����
 * @param[in]  nums д�����ܳ���
 * @param[in]  fd �򿪵��ļ���������Ҳ�������׽���
 * @param[out] p_buf д���ݻ�����
 * @return	 ������ERROR�����򷵻�ʵ��д������ݳ��ȣ�����ָ��д��ĳ���
 */
INT32 writen(INT32 fd, const void *p_buf, INT32 nums)
{
	INT32 nleft = nums;
	INT32 nwritten = 0;
	const INT8* ptr = (const INT8*)p_buf;
	
	if (fd < 0 || NULL == p_buf || nums < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}

	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_INFO, "write: %s\n", strerror(errno));
				nwritten = 0;
			}
			else
			{
				DEBUG_PRINT(DEBUG_ERROR, "write failed: %s\n", strerror(errno));
				return ERROR;
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nums);
}

/**		  
 * @brief		�򿪲�����һ��Posix��Ϣ����
 * @param[in]  p_name ��Ϣ��������
 * @param[in]  maxmsg �����Ϣ��
 * @param[in] msgsize ������Ϣ������ֽ���
 * @return	 ������ERROR�����򷵻ش򿪵���Ϣ����������
 */
mqd_t message_queue_create(const INT8 *p_name, INT32 maxmsg, INT32 msgsize)
{
	mqd_t mqd = 0;
	struct mq_attr attr;
	
	if (NULL == p_name)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}
	
	memset(&attr, 0, sizeof(struct mq_attr));
	attr.mq_maxmsg = maxmsg;
	attr.mq_msgsize = msgsize;
	
	(void)mq_unlink(p_name);
	mqd = mq_open(p_name, O_RDWR | O_CREAT | O_EXCL, 0777, &attr);
	if (ERROR == mqd)
	{
		DEBUG_PRINT(DEBUG_ERROR, "mq_open failed: %s\n", strerror(errno));
		return ERROR;
	}
	
	return mqd;
}

/**		  
 * @brief		 ��������Posix��Ϣ�����е�һ����Ϣ
 * @param[in]  mqd ��Ϣ����������
 * @param[in]  buff_len ���ջ���������
 * @param[out] p_msg_buff ������Ϣ�Ļ�����
 * @return	 ������ERROR�����򷵻�ʵ�ʽ��յ���Ϣ�ĳ���
 */
INT32 message_queue_receive(mqd_t mqd, void *p_msg_buff, INT32 buff_len)
{
	INT32 recv_len = 0;
	
	if (mqd < 0 || NULL == p_msg_buff || buff_len < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}
	
	FOREVER
	{
		recv_len = mq_receive(mqd, (INT8 *)p_msg_buff, buff_len, NULL);
		if (ERROR == recv_len)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_INFO, "mq_receive: %s\n", strerror(errno));
				continue;
			}
			
			DEBUG_PRINT(DEBUG_ERROR, "mq_receive failed: %s\n", strerror(errno));
			return ERROR;
		}
		else
		{
			return recv_len;
		}
	}
}

/**		  
 * @brief		 ��������һ����Ϣ��Posix��Ϣ����
 * @param[in]  mqd ��Ϣ����������
 * @param[in]  p_msg ��Ҫ���͵���Ϣ
 * @param[in] msg_len ��Ϣ����
 * @param[in] priority ������Ϣ�����ȼ�,ֵԽ�����ȼ�Խ��
 * @return	 �ɹ�����OK,������ERROR
 */
INT32 message_queue_send_with_priority(mqd_t mqd, const void *p_msg, INT32 msg_len, UINT32 priority)
{
	if (mqd < 0 || NULL == p_msg || msg_len < 0)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
		return ERROR;
	}
	
	FOREVER
	{
		if (mq_send(mqd, (INT8 *)p_msg, msg_len, priority) == ERROR)
		{
			if (EINTR == errno)
			{
				DEBUG_PRINT(DEBUG_INFO, "mq_send: %s\n", strerror(errno));
				continue;
			}
			
			DEBUG_PRINT(DEBUG_ERROR, "mq_send failed: %s\n", strerror(errno));
			return ERROR;
		}
		else
		{
			return OK;
		}
	}
}

/**		  
 * @brief		 ��������һ����Ϣ��Posix��Ϣ����
 * @param[in]  mqd ��Ϣ����������
 * @param[in]  p_msg ��Ҫ���͵���Ϣ
 * @param[in] msg_len ��Ϣ����
 * @return	 �ɹ�����OK,������ERROR
 */
INT32 message_queue_send(mqd_t mqd, const void *p_msg, INT32 msg_len)
{
	return message_queue_send_with_priority(mqd, p_msg, msg_len, 0);
}

/**		  
 * @brief		 ����׽��ֽ��ջ������Ƿ�Ϊ��
 * @param[in]  sock_fd �Ѵ򿪵��׽���
 * @return	 TRUE-�գ�FALSE-�ǿ�
 */
BOOL socket_recv_empty(INT32 sock_fd)
{
	fd_set rset;
	struct timeval timeout;

	FD_ZERO(&rset);
	FD_SET(sock_fd, &rset);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select(sock_fd + 1, &rset, NULL, NULL, &timeout) > 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

