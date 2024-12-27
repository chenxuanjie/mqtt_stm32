#include "stm32f10x.h"                  // Device header
#include "MQTT.h"
#include "TIM3.h"
#include "esp8266.h"
#include "usart.h"
#include <stdlib.h>

unsigned char *dataPtr2 = NULL;

int main(void)
{
//	TIM3_Init();
	MQTT_Init();
	
	MQTT_Publish(15, 31);
	while(1)
	{
		dataPtr2 = ESP8266_Recv(100);
		
		if(dataPtr2 != NULL)
			UsartPrintf(USART1, "%s\r\n", dataPtr2);
	}
}
