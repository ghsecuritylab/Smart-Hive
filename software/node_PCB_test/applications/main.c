/*
   ------------  Smart Hive System  ---------------
 * Date           Author       Notes                                              		 Defects 
 * 2018-11-30      Drfish       create -> ADD LED_thread
 * 2018-12-01      Drfish       create -> ADD KEY Control_IO
 * 2018-12-02      Drfish       create -> ADD DS18B20                                  The temperature seems to be a little high
 * 2018-12-03      Drfish       create -> ADD HX711 Weight_collect                     The weight hasn't been fitted yet
 * 2018-12-04      Drfish       create -> WIFI Connect & Voltage_AD_collect      
 * 2018-12-05      Drfish       create -> Upper Computer Communication Normal      		 Temperature data is unstable on the Upper Computer
 * 2018-12-05      Drfish       create -> ADD BEEP & AD_Voltage Filtering
 * 2018-12-06      Drfish       create -> ADD OLED Display  &  filter_temp 
 * 2018-12-07      Drfish       create -> Improve variable struct
 * 2018-12-10      Drfish       create -> ADD  SFUD  FLASH�洢 ������״̬ & ���������

 Unfinished tasks:1.  ADD FLASH �洢  ����״̬��Ƣ�� ����״̬    ����������̡�
									2.  ADD Servo Motor Control     
									
*/
 

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "my_init.h"
#include "sfud.h"
#include "string.h"
#include "spi_flash.h"
#include "spi_flash_sfud.h"

extern rt_int32_t Weight[3];
extern rt_uint16_t Temperature[3]; //�¶�
extern rt_uint16_t voltage[3];     //��ѹ
extern rt_uint8_t DataBuffer[8];   //���ݰ�

extern rt_uint8_t FW_status_flag;
extern rt_uint8_t PiShu;
extern bool   HEAT_START_FLAG;          //��ʼ���Ƽ���˿���ȱ�־λ
extern bool   WASHING_START_FLAG;       //��ʼ��ϴ�����־λ
extern bool   FEED_START_FLAG;          //��ʼιʳ�����־λ

rt_spi_flash_device_t W25Q128_dev = NULL;
const sfud_flash *flash = NULL;



int main(void)
{
//	rt_uint8_t temp; 


//	W25Q128_dev = rt_sfud_flash_probe("flash0", "norspi");
//	flash = (sfud_flash_t)W25Q128_dev->user_data;
//	sfud_read(flash,0,sizeof(FW_status_flag), &FW_status_flag);
//	sfud_read(flash,1,sizeof(PiShu), &PiShu);      //��ʼ����� W25Q128 FLASH����������״̬����Ƣ����
				
	
	while(1)
	{
		//rt_thread_delay(100);  //1s
		rt_pin_write(Heat1_PIN, PIN_HIGH);rt_kprintf("Heat1_ON!\t");
		rt_pin_write(Watering_PIN, PIN_HIGH);rt_kprintf("Water_ON!\t");
		
		rt_thread_delay(50);  //1s
		
		rt_pin_write(Heat2_PIN, PIN_HIGH);rt_kprintf("Heat2_ON!\t");
		rt_pin_write(Feeding_PIN, PIN_HIGH);rt_kprintf("Feed_ON!\t");
		
		rt_thread_delay(100);  //1s
		rt_pin_write(Heat1_PIN, PIN_LOW);rt_kprintf("Heat1_ON!\t");
		rt_pin_write(Watering_PIN, PIN_LOW);rt_kprintf("Water_ON!\t");
		rt_pin_write(Heat2_PIN, PIN_LOW);rt_kprintf("Heat2_ON!\t");
		rt_pin_write(Feeding_PIN, PIN_LOW);rt_kprintf("Feed_ON!\t");
		rt_thread_delay(100);  //1s
//		DataBuffer[0]=(Weight[0]*100);                  //2�ֽڵķ����������ݷ�������
//		DataBuffer[1]=(Weight[0]*100)>>8;
//		DataBuffer[2]=0xFF;                             //2�ֽڵ������������ݷ�������(ffff�������ϳ���)
//		DataBuffer[3]=0xFF;
//		DataBuffer[5]=Temperature[0]+500;               //2�ֽڵ��¶����ݷ���������
//		DataBuffer[4]=(Temperature[0]+500)>>8;
//	
//		DataBuffer[6]=HEAT_START_FLAG;                     //�����ȱ�־λ�������飬���忴ͨ�����ݸ�ʽ
//		DataBuffer[6]=DataBuffer[6]<<1;                    //00000010�������ڼ���
//		DataBuffer[6]=DataBuffer[6]+FEED_START_FLAG;        //��ιʳ��־λ�������飬���忴ͨ�����ݸ�ʽ
//		temp=WASHING_START_FLAG;
//		DataBuffer[6]=DataBuffer[6]+(temp<<2);
	
		rt_thread_mdelay(10); 
	}

}



void WIFI_SEND_BUFF(void* parameter)  //WIFI����λ��ͨ��
{

	while(1)
	{	
		Uart2DataAnalyzing(); //WIFI���ݰ�����
		rt_thread_delay(5);  //10ms
	}
	
}

int wifi_send_init()
{
	rt_thread_t tid;
        tid = rt_thread_create("WIFI_SEND_BUFF",WIFI_SEND_BUFF, RT_NULL,1024,2, 20);
        if (tid != RT_NULL)
                rt_thread_startup(tid);
        
        return 0;
}
INIT_APP_EXPORT(wifi_send_init);


