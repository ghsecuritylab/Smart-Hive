#ifndef __DS18B20_H
#define __DS18B20_H 

#include <rtthread.h>
#include <board.h>
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK MiniSTM32������
//DS18B20��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2014/3/12
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

#define DS18B20_PIN 54
//IO��������
#define DS18B20_IO_IN()  {rt_pin_mode(DS18B20_PIN, PIN_MODE_INPUT);} //PD2
#define DS18B20_IO_OUT() {rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);}
////IO��������											   
#define	DS18B20_DQ_OUT_HIGH rt_pin_write(DS18B20_PIN, PIN_HIGH)//���ݶ˿�	PD2
#define	DS18B20_DQ_OUT_LOW  rt_pin_write(DS18B20_PIN, PIN_LOW)	//���ݶ˿�	PD2
#define	DS18B20_DQ_IN  rt_pin_read(DS18B20_PIN)  //���ݶ˿�	PD2 
   	
rt_uint8_t DS18B20_Init(void);			//��ʼ��DS18B20
short DS18B20_Get_Temp(void);	//��ȡ�¶�
void DS18B20_Start(void);		//��ʼ�¶�ת��
void DS18B20_Write_Byte(rt_uint8_t dat);//д��һ���ֽ�
rt_uint8_t DS18B20_Read_Byte(void);		//����һ���ֽ�
rt_uint8_t DS18B20_Read_Bit(void);		//����һ��λ
rt_uint8_t DS18B20_Check(void);			//����Ƿ����DS18B20
void DS18B20_Rst(void);			//��λDS18B20    
#endif















