#include "my_init.h"
/*
˵����
			����5s��ʼ���ɼ� �������(����Сֵ)���󴮿�1 ��ӡ����Please place 1kg!��  Ȼ���ڷ�����
			����1kg�����ϵͳ�ɼ� 0��1kg������Kֵ��
			���°���KEY3����ʾ��һ���ɼ��ɹ���
*/



extern rt_uint32_t Count;
extern rt_uint32_t HX711_Date[2];

extern rt_uint8_t Init_flag;
extern rt_uint8_t first_ON_OFF;
extern rt_uint16_t Test_count;

extern rt_int32_t Init_Value[2];
extern rt_int32_t AD_Value[2];
extern rt_int32_t Last_AD_Value[2];
extern rt_int32_t side_weight[2];
extern rt_int32_t Weight[3];

extern rt_int32_t temp[2];
extern rt_int32_t var_max[2];    //8632784        //�����ֵ�� ��Ϊ��һ�����ֵ
extern rt_int32_t var_min[2];  //�����ֵС ��Ϊ��һ����Сֵ  -2147483648��+2141483647
extern rt_int32_t var0[10];
extern rt_int32_t var1[10];
extern rt_int32_t Real_Weight;

extern rt_uint8_t set1kg_flag ;
extern float Normalized[2];



rt_uint32_t HX711_ReadAD1(void)                               //��ȡHX711ģ��ɼ�����ֵ	
{
  unsigned char i;
  HX711_SCK1_LOW;                                                //ʹ��AD(PD_SCK ����)
  Count=0;
  while(HX711_DT1);                                           //ADת��δ������ȴ�,����ʼ��ȡ
  for(i=0;i<24;i++)
   {
      HX711_SCK1_HIGH;                                            //PD_SCK �ø�(��������)
      Count=Count<<1;                                    //�½�������ʱ����Count����һλ,������
      HX711_SCK1_LOW;                                            //PD_SCK ����
      if(HX711_DT1) Count++;
     }
  HX711_SCK1_HIGH;
  Count=Count^0x800000;                                  //��25�������½�����ʱ,ת���������
  HX711_SCK1_LOW;
  return(Count);
}

rt_uint32_t HX711_ReadAD2(void)                               //��ȡHX711ģ��ɼ�����ֵ	
{
  unsigned char i;
  HX711_SCK2_LOW;                                                //ʹ��AD(PD_SCK ����)
  Count=0;
  while(HX711_DT2);                                           //ADת��δ������ȴ�,����ʼ��ȡ
  for(i=0;i<24;i++)
   {
      HX711_SCK2_HIGH;                                            //PD_SCK �ø�(��������)
      Count=Count<<1;                                    //�½�������ʱ����Count����һλ,������
      HX711_SCK2_LOW;                                            //PD_SCK ����
      if(HX711_DT2) Count++;
     }
  HX711_SCK2_HIGH;
  Count=Count^0x800000;                                  //��25�������½�����ʱ,ת���������
  HX711_SCK2_LOW;
  return(Count);
}



void weight_get_val(void* parameter)
{
		rt_uint8_t i=0;
		rt_pin_mode(HX711_DT1_PIN, PIN_MODE_INPUT_PULLUP);  
		rt_pin_mode(HX711_SCK1_PIN, PIN_MODE_OUTPUT);
		rt_pin_write(HX711_SCK1_PIN, PIN_HIGH);
	
		rt_pin_mode(HX711_DT2_PIN, PIN_MODE_INPUT_PULLUP);  
		rt_pin_mode(HX711_SCK2_PIN, PIN_MODE_OUTPUT);
		rt_pin_write(HX711_SCK2_PIN, PIN_HIGH);


	
		while(1)
		{
			Last_AD_Value[0]=AD_Value[0];
			Last_AD_Value[1]=AD_Value[1];
			for(i=0;i<10;i++){
				 var0[i]=HX711_ReadAD1();
				 var1[i]=HX711_ReadAD2();
			}
			AD_Value[0]=Bubble(var0);
			AD_Value[1]=Bubble(var1);
			
			
			
//			side_weight[0] = Normalized[0]*(AD_Value[0]-var_min[0])*10;  //�Ŵ�10��  //���ȿ�����
//			side_weight[1] = Normalized[1]*(AD_Value[1]-var_min[1])*10;  //�Ŵ�10��
//			
//			Weight[0] = (side_weight[0]+side_weight[1])/2;   //��ʵ����
//			
//			Weight[1] = (Weight[0])/10;   //ȡ����λ  ��λ 1kg 
//			Weight[2] = (Weight[0])%10;   //ȡС��λ  ��λ 10g
			
//			if(side_weight[0]>0){
//				rt_kprintf("\r\nside_Weight[0] :%d   ",side_weight[0]);
//			}
//			else {rt_kprintf("\r\nside_Weight[0] :%d   ",side_weight[0]);}
//			
//			if(side_weight[1]>0){
//				rt_kprintf("nside_Weight[1] :%d   ",side_weight[1]);
//			}
//			else {rt_kprintf("nside_Weight[1] :%d   ",side_weight[1]);}
//			
//			rt_kprintf("\r\nWeight[0] :%d.%d kg",Weight[1],Weight[2]);
			rt_thread_delay(100);     
			
		}
}



int hx711_thread_init()
{
		rt_thread_t hx711_tid;
    hx711_tid = rt_thread_create("hx711",
                    weight_get_val,
                    (void*)5,
                    1024,
                    2,
                    10);
    if (hx711_tid != RT_NULL){
			rt_thread_startup(hx711_tid);}
		return 0;
}
INIT_APP_EXPORT(hx711_thread_init);





