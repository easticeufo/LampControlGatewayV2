      
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

static INT32 debug_level = DEBUG_ERROR; // 调试打印信息输出等级

/**		  
 * @brief 获取调试打印信息输出等级
 * @param 无
 * @return 调试打印信息输出等级
 */
INT32 get_debug_level(void)
{
	return debug_level;
}

/**		  
 * @brief		设置调试打印信息输出等级 
 * @param[in] level 调试打印信息输出等级
 * @return 无
 */
void set_debug_level(INT32 level)
{
	debug_level = level;
}

/**
 * @brief 获取程序编译日期，返回一个整数表示的日期,若p_date_buff不是NULL，在p_date_buff中返回编译日期的字符串
 * @param[out] p_date_buff 将日期按照格式"build yyyymmdd"存放
 * @param[in] buff_len 存放编译日期字符串的缓冲区长度
 * @return 返回一个整数表示日期,16~31位表示年份,8~15位表示月份,0~7位表示日期
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
 * @brief		创建线程函数封装  
 * @param[in] p_func 线程函数指针
 * @param[in] arg 传递给线程回调函数的参数
 * @return	成功返回OK，失败返回ERROR
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
	
	/* 设置分离线程属性 */
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
 * @brief		 以UINT8格式计算缓冲区内数据的校验和
 * @param[in]  p_data 数据缓冲区
 * @param[in]  nums 数据长度
 * @return	 校验和,输入参数错误返回ERROR(0xffffffff)
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
 * @brief		 读指定长度的数据，若超时或连接断开或读到文件尾则返回，否则阻塞
 * @param[in]  nums 读数据总长度
 * @param[in]  fd 打开的描述符
 * @param[out] p_buf 读数据缓冲区
 * @return	 出错返回ERROR，否则返回实际读到的数据长度
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
				/* 连接关闭或读到文件尾 */
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
 * @brief		 阻塞写指定长度的数据
 * @param[in]  nums 写数据总长度
 * @param[in]  fd 打开的文件描述符，也可用于套接字
 * @param[out] p_buf 写数据缓冲区
 * @return	 出错返回ERROR，否则返回实际写入的数据长度，等于指定写入的长度
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
 * @brief		打开并创建一个Posix消息队列
 * @param[in]  p_name 消息队列名称
 * @param[in]  maxmsg 最大消息数
 * @param[in] msgsize 单条消息的最大字节数
 * @return	 出错返回ERROR，否则返回打开的消息队列描述符
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
 * @brief		 阻塞接收Posix消息队列中的一条消息
 * @param[in]  mqd 消息队列描述符
 * @param[in]  buff_len 接收缓冲区长度
 * @param[out] p_msg_buff 接收消息的缓冲区
 * @return	 出错返回ERROR，否则返回实际接收的消息的长度
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
 * @brief		 阻塞发送一条消息到Posix消息队列
 * @param[in]  mqd 消息队列描述符
 * @param[in]  p_msg 需要发送的消息
 * @param[in] msg_len 消息长度
 * @param[in] priority 发送消息的优先级,值越大优先级越高
 * @return	 成功返回OK,出错返回ERROR
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
 * @brief		 阻塞发送一条消息到Posix消息队列
 * @param[in]  mqd 消息队列描述符
 * @param[in]  p_msg 需要发送的消息
 * @param[in] msg_len 消息长度
 * @return	 成功返回OK,出错返回ERROR
 */
INT32 message_queue_send(mqd_t mqd, const void *p_msg, INT32 msg_len)
{
	return message_queue_send_with_priority(mqd, p_msg, msg_len, 0);
}

/**		  
 * @brief		 检查套接字接收缓冲区是否为空
 * @param[in]  sock_fd 已打开的套接字
 * @return	 TRUE-空，FALSE-非空
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

