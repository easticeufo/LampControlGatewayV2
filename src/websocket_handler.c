      
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-7-10
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "appweb.h"

#define MAX_WEBSOCKET_LINK 10 // websocket最多连接数

extern INT32 check_auth_permission(HttpConn *conn);

static HttpConn *websocket_statechange_conn[MAX_WEBSOCKET_LINK] = {NULL};

/**		  
 * @brief 获取已经建立websocket连接的客户端个数
 * @param 无
 * @return 返回连接数
 */
INT32 websocket_get_link_num(void)
{
	INT32 i = 0;
	INT32 num = 0;

	for (i = 0; i < MAX_WEBSOCKET_LINK; i++)
	{
		if (websocket_statechange_conn[i] != NULL)
		{
			num++;
		}
	}
	
	return num;
}

/**		  
 * @brief		将消息发送给所有已连接的websocket的客户端
 * @param[in] p_msg 要发送的消息字符串
 * @return	无
 */
void websocket_statechange_send_msg(const INT8 *p_msg)
{
	INT32 i = 0;

	for (i = 0; i < MAX_WEBSOCKET_LINK; i++)
	{
		if (websocket_statechange_conn[i] != NULL)
		{
			httpSend(websocket_statechange_conn[i], "%s", p_msg);
		}
	}
	
	return;
}

static INT32 delete_statechange_conn(HttpConn *conn)
{
	INT32 i = 0;
	
	if (NULL == conn)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	for (i = 0; i < MAX_WEBSOCKET_LINK; i++)
	{
		if (websocket_statechange_conn[i] == conn)
		{
			websocket_statechange_conn[i] = NULL;
			break;
		}
	}

	if (i >= MAX_WEBSOCKET_LINK)
	{
		DEBUG_PRINT(DEBUG_WARN, "websocket conn delete: can't find the connected websocket!\n");
	}
	
	return OK;
}

static INT32 add_statechange_conn(HttpConn *conn)
{
	INT32 i = 0;
	INT32 add_idx = -1;
	
	if (NULL == conn)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return ERROR;
	}

	for (i = 0; i < MAX_WEBSOCKET_LINK; i++)
	{
		if (websocket_statechange_conn[i] == conn) // 已经添加过一次了，重复添加
		{
			DEBUG_PRINT(DEBUG_ERROR, "this conn has already been added\n");
			add_idx = i;
			break;
		}

		if (-1 == add_idx && websocket_statechange_conn[i] == NULL)
		{
			add_idx = i;
		}
	}

	if (-1 == add_idx) // 已经达到最大连接数，无法新增连接
	{
		DEBUG_PRINT(DEBUG_WARN, "exceed max websocket link, can't create new websocket connect!\n");
		return ERROR;
	}

	websocket_statechange_conn[add_idx] = conn;
	return OK;
}

void websocket_statechange_callback(HttpConn *conn, int event, int arg)
{
	HttpPacket *packet;

	if (NULL == conn || NULL == conn->readq)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return;
	}

	if (event == HTTP_EVENT_READABLE)
	{
		packet = httpGetPacket(conn->readq);
		DEBUG_PRINT(DEBUG_NOTICE, "/websocket/statechange receive:%s\n", httpGetPacketStart(packet));
	}
	else if (event == HTTP_EVENT_APP_CLOSE)
	{
		DEBUG_PRINT(DEBUG_NOTICE, "/websocket/statechange closed!\n");
		(void)delete_statechange_conn(conn);
	}
	else if (event == HTTP_EVENT_ERROR)
	{
		DEBUG_PRINT(DEBUG_WARN, "/websocket/statechange error!\n");
	}
	else
	{
		DEBUG_PRINT(DEBUG_INFO, "websocket_statechange_callback event=%d\n", event);
	}

	return;
}


static void websocket_start(HttpQueue *q)
{
	if (NULL == q || NULL == q->conn || NULL == q->conn->rx)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
        return;
	}

	DEBUG_PRINT(DEBUG_NOTICE, "websocket_start uri=%s\n", q->conn->rx->uri);

	/* 用户权限验证 */
	if (check_auth_permission(q->conn) == -1)
	{
		httpError(q->conn, HTTP_CODE_UNAUTHORIZED, "Access Denied. Login required");
		DEBUG_PRINT(DEBUG_NOTICE, "unauthorized!\n");
		return;
	}

	if (httpGetWebSocketState(q->conn) != WS_STATE_OPEN) // 非websocket连接返回错误
	{
		DEBUG_PRINT(DEBUG_WARN, "this is not websocket connection\n");
		httpError(q->conn, HTTP_CODE_BAD_REQUEST, "must be websocket connection");
		return;
	}
	
	if (strcmp(q->conn->rx->uri, "/websocket/statechange") == 0)
	{
		if (add_statechange_conn(q->conn) != OK)
		{
			httpSendClose(q->conn, WS_STATUS_POLICY_VIOLATION, "exceed max link");
			return;
		}
		httpSetConnNotifier(q->conn, websocket_statechange_callback);
	}
	else
	{
		DEBUG_PRINT(DEBUG_WARN, "not support websocket uri:%s\n", q->conn->rx->uri);
		httpSendClose(q->conn, WS_STATUS_PROTOCOL_ERROR, "unsupported uri");
	}

	return;
}

INT32 websocket_handler_init(Http *http)
{
	HttpStage   *handler = NULL;

	if (NULL == http)
	{
		DEBUG_PRINT(DEBUG_ERROR, "param error\n");
	    return ERROR;
	}

	handler = httpCreateHandler(http, "websocketHandler", NULL);
	if (NULL == handler)
	{
		DEBUG_PRINT(DEBUG_ERROR, "httpCreateHandler failed!\n");
	    return ERROR;
	}

	handler->start = websocket_start;

	return OK;
}

