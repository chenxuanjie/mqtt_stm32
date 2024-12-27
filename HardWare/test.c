void UART4_Init(uint32_t bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);	//使能UART4，GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	USART_DeInit(UART4);  //复位串口4
	
	//UART4_TX   PC.10 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_Init(GPIOC, &GPIO_InitStructure); //初始化PC10
   
	//UART4_RX	  PC.11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	GPIO_Init(GPIOC, &GPIO_InitStructure);  //初始化PC11
 
	//USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;	//设置波特率，一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;	//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	
	USART_Init(UART4, &USART_InitStructure); //初始化串口
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//开启中断
	USART_Cmd(UART4, ENABLE);                    //使能串口 
 
 
	TIM7_Int_Init(100-1,7200-1);		//10ms 为了判断是不是一串数据后文有解释
 
	TIM_Cmd(TIM7,ENABLE);			//
}

//定时器7中断服务程序		    
void TIM7_IRQHandler(void)
{ 	
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)//是更新中断
	{
	
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update  );  //清除TIM7更新中断标志    
	    if(F_UART4_RX_RECEIVING)//正在接收串口数据
		{
			UART4_RX_TIMEOUT_COUNT++;//串口超时计数
			if(UART4_RX_TIMEOUT_COUNT>3)//数据接收间隔超过30ms
			{//串口接收完成或结束
				F_UART4_RX_RECEIVING=0;
				UART4_RX_TIMEOUT_COUNT=0;
				F_UART4_RX_FINISH=1;
			}
		}
	}	    
}
 
 
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz 
//通用定时器中断初始化
//这里始终选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数		 
void TIM7_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7时钟使能    
	
	//定时器TIM7初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); //使能指定的TIM7中断,允许更新中断
	
	TIM_Cmd(TIM7,ENABLE);//开启定时器7
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
}

#define AT_CWMODE	"AT+CWMODE=1" //设置为“station”模式
int8_t ESP8266_SetStation(void)
{
	ClrAtRxBuf();//清空缓存
	SendAtCmd((uint8_t *)AT_CWMODE,strlen(AT_CWMODE));
	delay_ms(100);
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		return -1;
	}
	return 0;
}

/*******************************************************************
*函数：int8_t ESP8266_SetAP(void)
*功能：设置ESP8266要连接的热点名称和密码
*输入：char *wifi-热点名称 char *pwd-热点密码
*输出：
		return = 0 ,sucess
		return < 0 ,error
*特殊说明：
*******************************************************************/
#define WIFI_AP		"dmx"//WiFi热点名称       
#define WIFI_PWD	"88888888"	//WiFi密码
 
int8_t ESP8266_SetAP(void)
{
	uint8_t AtCwjap[MAX_AT_TX_LEN];
	memset(AtCwjap, 0x00, MAX_AT_TX_LEN);//清空缓存
	ClrAtRxBuf();//清空缓存
	sprintf((char *)AtCwjap,"AT+CWJAP=\"%s\",\"%s\"",WIFI_AP, WIFI_PWD	);
	SendAtCmd((uint8_t *)AtCwjap,strlen((const char *)AtCwjap));
	delay_ms(5500);
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		return -1;
	}
	return 0;
}

#define SERVER_PC_IP	"172.16.40.36"	//服务器IP地址
#define SERVER_PC_PORT	1883			//服务器端口号   
int8_t ESP8266_IpStart(void)
{
	uint8_t IpStart[MAX_AT_TX_LEN];
	memset(IpStart, 0x00, MAX_AT_TX_LEN);//清空缓存
	ClrAtRxBuf();//清空缓存
	
	/**************配置MQTT_USER信息***************/
	//AT+MQTTUSERCFG=0,1,"用户ID","账号","密码",0,0,""
	                       //AT+MQTTUSERCFG=0,1,\"M3\",\"\",\"\",0,0,\"\"
	sprintf((char *)IpStart,"AT+MQTTUSERCFG=0,1,\"M3\",\"\",\"\",0,0,\"\"");
	SendAtCmd((uint8_t *)IpStart,strlen((const char *)IpStart));
	delay_ms(1500);
	printf("IpStart-USER_Config: %s\r\n", (char *)AT_RX_BUF);
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		/******断开连接*****/
		memset(IpStart, 0x00, MAX_AT_TX_LEN);//清空缓存
		ClrAtRxBuf();//清空缓存
		
		//AT+MQTTCLEAN=0
		sprintf((char *)IpStart,"AT+MQTTCLEAN=0");
		
		SendAtCmd((uint8_t *)IpStart,strlen((const char *)IpStart));
		
		return -1;
	}
	
	/**************建立MQTT连接***************/
	memset(IpStart, 0x00, MAX_AT_TX_LEN);//清空缓存
	ClrAtRxBuf();//清空缓存
	
	//AT+MQTTCONN=0,"服务器IP",1883,0
	//AT+MQTTCONN=0,"172.16.40.36",1883,0
	sprintf((char *)IpStart,"AT+MQTTCONN=0,\"%s\",%d,0",SERVER_PC_IP, SERVER_PC_PORT	);
	
	SendAtCmd((uint8_t *)IpStart,strlen((const char *)IpStart));
	delay_ms(1500);
	printf("IpStart-MQTT_Connect: %s\r\n", (char *)AT_RX_BUF);
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		return -2;
	}
	
	return 0;
}

int8_t ESP8266_MQTT_Sub(char *topic) //主题自己设置
{
	int8_t error = 0;
	uint8_t IpSend[MAX_AT_TX_LEN];
	memset(IpSend, 0x00, MAX_AT_TX_LEN);//清空缓存
	ClrAtRxBuf();//清空缓存
	
	
	//AT+MQTTSUB=0,M3",0
	sprintf((char *)IpSend,"AT+MQTTSUB=0,\"%s\",1", topic);
	SendAtCmd((uint8_t *)IpSend,strlen((const char *)IpSend));
	delay_ms(1500);
	printf("MQTT_Sub_Ack: %s\r\n",AT_RX_BUF);
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		return -4;
	}
	
	return error;
}

int8_t ESP8266_MQTT_Pub(char *IpBuf, uint8_t len, uint8_t qos)//信息 消息长度 以及qos(百度)
{
	uint8_t TryGo = 0;
	int8_t error = 0;
	uint8_t IpSend[MAX_AT_TX_LEN];
	memset(IpSend, 0x00, MAX_AT_TX_LEN);//清空缓存
	ClrAtRxBuf();//清空缓存
	/************设置为MQTT_Binary发送模式************/
/*   注意----------dmx为发布的主题-----------*/
	sprintf((char *)IpSend,"AT+MQTTPUBRAW=0,\"dmx\",%d,%d,0", len, qos);
	//printf("%s\r\n",IpSend);
	SendAtCmd((uint8_t *)IpSend,strlen((const char *)IpSend));
	delay_ms(100);
	//注意--如果断开连接或者怎么样，这一步ESP8266就会返回ERROR
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		printf("MQTT_Pub Fail:%s\r\n", AT_RX_BUF);
		MQTTSendErrorTimes++;
		return -1;
	}
	
	ClrAtRxBuf();//清空缓存
	
	//以二进制数据形式发送字符串
	SendAtCmd((uint8_t *)IpBuf, len);
	printf("MQTT_Pub: %s\r\n", IpBuf);
	if(qos == 0){
		return 0;
	}
	
	//等待qos回执处理
	delay_ms(100);
	if(strstr((const char *)AT_RX_BUF, (const char *)"OK") == NULL)
	{
		MQTTSendErrorTimes++;
		return -2;
	}
	return error;
}