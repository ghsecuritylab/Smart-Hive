#ifndef __MY_VARIABLE_H
#define __MY_VARIABLE_H



#define MAX_DATA_LENS 15

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"


typedef enum {FALSE = 0, TRUE = !FALSE} bool;


typedef enum {  //ö�ٷ���״̬
	ZhongWang,  //=0
	WangTai,
	ShiWang,
	ChuNuWang

}FW_status;//�������״̬ ����Ϊö������

typedef struct A //���� �ṹ��ȫ�ֱ���
{
  rt_uint8_t fx_number;  //������
  rt_uint8_t fw_status;  //����״̬
  rt_uint8_t pishu;      //Ƣ��
  rt_uint16_t temp;      //�¶�
	rt_uint16_t weight;    //����
  rt_uint32_t vol;       //��ѹ
 
}fx_date;






#endif

