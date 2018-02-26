
/**@file 
 * @note tiansixinxi. All Right Reserved.
 * @brief  
 * 
 * @author   madongfang
 * @date     2016-7-4
 * @version  V1.0.0
 * 
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note  
 * @warning  
 */

#include "base_fun.h"
#include "upnp.h"

#define DEFAULT_NOTIFY_EXPIRE_TIME (10 * 60) ///< 默认NOTIFY超时时间，单位是秒

/* 设备描述信息 */
INT8 *device_desc = "<?xml version=\"1.0\"?>\n"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n"
	"  <specVersion>\n"
	"    <major>1</major>\n"
	"    <minor>0</minor>\n"
	"  </specVersion>\n"
	"  <device>\n"
	"    <deviceType>urn:schemas-upnp-org:device:EmbeddedNetDevice:1</deviceType>\n"
	"    <friendlyName>LampControlGateway</friendlyName>\n"
	"    <manufacturer>UPNP</manufacturer>\n"
	"    <manufacturerURL>http://www.upnp.com</manufacturerURL>\n"
	"    <modelDescription>Lamp Control GateWay</modelDescription>\n"
	"    <modelName>GateWay</modelName>\n"
	"    <modelNumber>GW19871023</modelNumber>\n"
	"    <modelURL>http://www.upnp.com</modelURL>\n"
	"    <serialNumber>19871023</serialNumber>\n"
	"    <UDN>uuid:Upnp-1_0</UDN>\n"
	"    <serviceList>\n"
	"      <service>\n"
	"        <serviceType>urn:schemas-upnp-org:service:EmbeddedNetDeviceControl:1</serviceType>\n"
	"        <serviceId>urn:upnp-org:serviceId:EmbeddedNetDeviceControl</serviceId>\n"
	"        <controlURL>/</controlURL>\n"
	"        <eventSubURL>/</eventSubURL>\n"
	"        <SCPDURL>/</SCPDURL>\n"
	"      </service>\n"
	"    </serviceList>\n"
	"   <presentationURL>http://%s:80</presentationURL>\n"
	"</device>\n"
	"</root>\n";

/**
 * @brief     upnp事件回调函数，暂时没有使用
 * @param[in] event_type
 * @param[in] event
 * @param[in] cookie
 * @return    返回值未使用
 */
static INT32 upnp_event_fun(Upnp_EventType event_type, void *event, void *cookie)
{
	DEBUG_PRINT(DEBUG_ERROR, "upnp_event_fun is called, event_type=%d\n", event_type);
	
	return 0;
}

/**
 * @brief upnp服务
 * @param no_use 未使用
 * @return 无
 */
void *upnp_fun(void *no_use)
{
	INT32 ret = 0;
	INT8 *ip_address = NULL;
	UINT16 port = 0;
	INT8 desc_url[64] = {0};
	INT8 *upnp_root_dir = "."; // 当前目录为upnp的根目录
	INT8 *desc_file_name = "upnpdevicedesc.xml";
	INT8 desc_file_path[32] = {0};
	UpnpDevice_Handle upnp_device_handler = 0;
	FILE *fp = NULL;
	
	ret = UpnpInit(NULL, 0);
	if (ret != UPNP_E_SUCCESS)
	{
		DEBUG_PRINT(DEBUG_ERROR, "UpnpInit failed, ret=%d\n", ret);
		(void)UpnpFinish();
		return NULL;
	}

	ip_address = UpnpGetServerIpAddress();
	if (ip_address == NULL)
	{
		DEBUG_PRINT(DEBUG_ERROR, "UpnpGetServerIpAddress failed\n");
		(void)UpnpFinish();
		return NULL;
	}
	
	port = UpnpGetServerPort();
	if (0 == port)
	{
		DEBUG_PRINT(DEBUG_ERROR, "UpnpGetServerPort failed\n");
		(void)UpnpFinish();
		return NULL;
	}

	DEBUG_PRINT(DEBUG_NOTICE, "UPnP Initialized ip_address=%s port=%u\n", ip_address, port);

	ret = UpnpSetWebServerRootDir(upnp_root_dir);
	if (ret != UPNP_E_SUCCESS)
	{
		DEBUG_PRINT(DEBUG_ERROR, "UpnpSetWebServerRootDir failed, ret=%d\n", ret);
		(void)UpnpFinish();
		return NULL;
	}

	snprintf(desc_file_path, sizeof(desc_file_path), "%s/%s", upnp_root_dir, desc_file_name);
	fp = fopen(desc_file_path, "w");
	if (NULL == fp)
	{
		DEBUG_PRINT(DEBUG_ERROR, "fopen failed\n");
		(void)UpnpFinish();
		return NULL;
	}

	fprintf(fp, device_desc, ip_address);

	fclose(fp);

	/* 将设备注册为一个upnp根设备 */
	snprintf(desc_url, sizeof(desc_url), "http://%s:%d/%s", ip_address, port, desc_file_name);
	ret = UpnpRegisterRootDevice(desc_url, upnp_event_fun, NULL, &upnp_device_handler);
	if (ret != UPNP_E_SUCCESS)
	{
		DEBUG_PRINT(DEBUG_ERROR, "UpnpRegisterRootDevice failed, ret=%d\n", ret);
		(void)UpnpFinish();
		return NULL;
	}

	ret = UpnpSendAdvertisement(upnp_device_handler, DEFAULT_NOTIFY_EXPIRE_TIME);
	if (ret != UPNP_E_SUCCESS)
	{
		DEBUG_PRINT(DEBUG_ERROR, "UpnpSendAdvertisement failed, ret=%d\n", ret);
		(void)UpnpFinish();
		return NULL;
	}
	
	return NULL;
}

