#include "stm32f10x.h"                  // Device header
#include "esp8266.h"
#include "onenet.h"
#include "mqttkit.h"
#include "usart.h"	 
#include "Delay.h"	 
#include "stdio.h"	 


#define TOPIC		"/sys/k25rsw78UOt/mqtt_stm32/thing/event/property/post"
#define SUB_TOPIC	"/sys/k25rsw78UOt/mqtt_stm32/thing/service/property/set"
#define productKey	"k25rsw78UOt"
#define CLIENTID	"mqtt_stm32|securemode=2\\,signmethod=hmacsha1\\,timestamp=1735096276543|"
#define USERNAME	"mqtt_stm32&k25rsw78UOt"
#define PASSWORD	"9248B39886BEF0B6AF6A53220280A92C8051614F"

 char PUB_BUF[256];//上传数据的buf
 const char *topics[] = {"/iot/1273/pdz"};
 unsigned short timeCount = 0;	//发送间隔变量
 unsigned char *dataPtr = NULL;
 float length;

uint8_t Aliyun_Link(char* clientId, char* username, char* password)
{
	char data[256];
	
	// Link to MQTT
	UsartPrintf(USART_DEBUG, "5. MQTTUSERCFG\r\n");
	sprintf(data, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",clientId, username, password);
	while (ESP8266_SendCmd(data, "OK"))
	{
		Delay_ms(500);	
		UsartPrintf(USART_DEBUG, "5. MQTTUSERCFG\r\n");
	}		

	
	// Link to MQTT brocker
	UsartPrintf(USART_DEBUG, "6. MQTTCONN\r\n");
	sprintf(data, "AT+MQTTCONN=0,\"%s.iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883,0\r\n",productKey);
	while (ESP8266_SendCmd(data, "OK")) 
	{
		Delay_ms(500);	
		UsartPrintf(USART_DEBUG, "6. MQTTCONN\r\n");
	}
	
	return 0;
}
	 
void Aliyun_Subscribe(const char *topic)
{
	char data[256];
	
	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic);
	sprintf(data, "AT+MQTTSUB=0,\"%s\",0\r\n",topic);
	while (ESP8266_SendCmd(data, "OK"))
	{
		Delay_ms(500);	
		UsartPrintf(USART_DEBUG, "retry to subscribe...\r\n");
	}
	UsartPrintf(USART_DEBUG, "subscribe succeed.\r\n");
}

void MQTT_Init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	Usart1_Init(115200);	 	//串口初始化为9600
	Usart2_Init(115200);		//8266通讯串口
	ESP8266_Init();//8266初始化
	UsartPrintf(USART1,"HardWare init\r\n");      
//	while(OneNet_DevLink())//接入onenet
//	Delay_ms(500);	 
//	OneNet_Subscribe(topics, 1);
	while (Aliyun_Link(CLIENTID, USERNAME, PASSWORD))
		Delay_ms(500);
	Aliyun_Subscribe(SUB_TOPIC);
}

void MQTT_Publish(uint8_t Temp, uint8_t Humidity)
{
	UsartPrintf(USART1, "OneNet_Publish\r\n");
	sprintf(PUB_BUF,"{\\\"params\\\":{\\\"temp\\\":%d\\,\\\"humi\\\":%d}\\,\\\"version\\\":\\\"1.0.0\\\"}",Temp ,Humidity);
	OneNet_Publish(TOPIC ,PUB_BUF);
	
	ESP8266_Clear();
}

void MQTT_Check(void)
{
	dataPtr = ESP8266_GetIPD(3);//完成需要15个毫秒，三次循环，一次5个毫秒
	if(dataPtr != NULL)
		OneNet_RevPro(dataPtr);
//	delay_ms(10);    
}

//void TIM3_IRQHandler(void)
//{
//	static uint32_t cnt;
//	if(TIM_GetFlagStatus(TIM3,TIM_FLAG_Update))
//	{
//		cnt++
//		if (cnt > 500)
//		{
//			cnt = 0;
//			
//		}
////		TIM3_Handler();
//		
//		TIM_ClearFlag(TIM3,TIM_FLAG_Update);
//	}
//}
