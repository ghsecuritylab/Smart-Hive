#ifndef __MY_INIT_H
#define __MY_INIT_H

#include <rtthread.h>

#include "my_variable.h"  //�û�����
#include "my_adc.h"
#include "my_led.h"
#include "my_timer.h"
#include "my_key.h"
#include "my_uart.h"
#include "my_HX711.h"
#include "my_ds18b20.h"
#include "my_oled.h"


#define Heat1_PIN    4
#define Heat2_PIN    9

#define LED2 56
#define LED3 57

#define BEEP_PIN 59
#define Feeding_PIN 61
#define Watering_PIN 62



//-------------------------------�������ָ����Ϣ-------------------------------//

#define SET_WASHING        0x01                         //��ϴ����
#define SET_FEED           0x02                         //ιʳ�۷�
#define SET_TEMPTURE       0x03                         //�����¶ȣ�����˿����
#define RETRUN_MESSAGE     0x04                         //���ط������Ϣ
#define RETURN_TEMPTURE    0x05                          //���ط����¶�ֵ
#define CONTR_BEE_STATE1   0x06                         //���ڴ����ȫ���۷䶼��ͨ��
#define CONTR_BEE_STATE2   0x07                         //������ֹͨ��
#define CONTR_BEE_STATE3   0x08                         //������ֹͨ���ҿ�����С



void my_delay_us(unsigned int us);
void my_delay_ms(unsigned int ms);

void buzzer_once(void);
void buzzer_bibi(rt_uint8_t t,rt_uint8_t l);
void buzzer_ring(void);

rt_int32_t Bubble(rt_int32_t *v);
rt_uint16_t Bubble_filter(rt_uint16_t *v,rt_uint8_t NO); //NOΪ 1��Ϊ��ֵ�˲�   NOΪ����ʱΪ ȡ��С
char *myitoa(int value, char *string, int radix);

#endif



