#ifndef __MQTT_H__
#define __MQTT_H__

void MQTT_Init(void);
void MQTT_Publish(uint8_t Temp, uint8_t Humidity);
void MQTT_Check(void);

#endif
