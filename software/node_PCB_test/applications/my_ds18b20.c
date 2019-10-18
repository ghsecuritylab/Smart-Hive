
#include "my_init.h" 

extern rt_uint16_t Temperature[3];



//////////////////////////////////////////////////////////////////////////////////

//��λDS18B20
void DS18B20_Rst(void)	   
{                 
	DS18B20_IO_OUT(); //SET PA0 OUTPUT
  DS18B20_DQ_OUT_LOW; //����DQ
  my_delay_us(750);   //����750us
  DS18B20_DQ_OUT_HIGH; //DQ=1 
	my_delay_us(15);      //15us
}
//�ȴ�DS18B20�Ļ�Ӧ
//����1:δ��⵽DS18B20�Ĵ���
//����0:����
rt_uint8_t DS18B20_Check(void) 	   
{   
	rt_uint8_t retry=0;
	DS18B20_IO_IN();//SET PA0 INPUT	 
  while (DS18B20_DQ_IN&&retry<200)
	{
		retry++;
		my_delay_us(1);   //1us
	};	 
	if(retry>=200)return 1;
	else retry=0;
  while (!DS18B20_DQ_IN&&retry<240)
	{
		retry++;
		my_delay_us(1);   //1us
	};
	if(retry>=240)return 1;	    
	return 0;
}
//��DS18B20��ȡһ��λ 

//����ֵ��1/0
rt_uint8_t DS18B20_Read_Bit(void) 			 // read one bit
{
    rt_uint8_t data;
	DS18B20_IO_OUT();//SET PA0 OUTPUT
  DS18B20_DQ_OUT_LOW; 
	my_delay_us(2); //2us
  DS18B20_DQ_OUT_HIGH; 
	DS18B20_IO_IN();//SET PA0 INPUT
	my_delay_us(12); //12us
	if(DS18B20_DQ_IN){data=1;}
    else {data=0;}	 
    my_delay_us(50);  //50us          
    return data;
}
//��DS18B20��ȡһ���ֽ�
//����ֵ������������
rt_uint8_t DS18B20_Read_Byte(void)    // read one byte
{        
  rt_uint8_t i,j,dat;
  dat=0;
	for (i=1;i<=8;i++) 
	{
     j=DS18B20_Read_Bit();
     dat=(j<<7)|(dat>>1);
  }						    
  return dat;
}
//дһ���ֽڵ�DS18B20
//dat��Ҫд����ֽ�
void DS18B20_Write_Byte(rt_uint8_t dat)     
 {             
   rt_uint8_t j;
   rt_uint8_t testb;
	 DS18B20_IO_OUT();//SET PA0 OUTPUT;
   for (j=1;j<=8;j++) 
	 {
        testb=dat&0x01;
        dat=dat>>1;
        if (testb) 
        {
            DS18B20_DQ_OUT_LOW;// Write 1   
            my_delay_us(2);     //2us                 
            DS18B20_DQ_OUT_HIGH; 
            my_delay_us(60);      //60us   
        }
        else 
        {
            DS18B20_DQ_OUT_LOW;// Write 0
            my_delay_us(60);    //60us             
            DS18B20_DQ_OUT_HIGH; 
             my_delay_us(2);   //2us                    
        }
    }
}
//��ʼ�¶�ת��
void DS18B20_Start(void)// ds1820 start convert
{   						               
    DS18B20_Rst();	   
		DS18B20_Check();	 
    DS18B20_Write_Byte(0xcc);// skip rom
    DS18B20_Write_Byte(0x44);// convert
} 
//��ʼ��DS18B20��IO�� DQ ͬʱ���DS�Ĵ���
//����1:������
//����0:����    	 
rt_uint8_t DS18B20_Init(void)
{
	rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);
	rt_pin_write(DS18B20_PIN, PIN_HIGH);

	DS18B20_Rst();

	return DS18B20_Check();
}  
//��ds18b20�õ��¶�ֵ
//���ȣ�0.1C
//����ֵ���¶�ֵ ��-550~1250�� 
short DS18B20_Get_Temp(void)
{
    rt_uint8_t temp;
    rt_uint8_t TL,TH;
		short tem;
    DS18B20_Start ();                    // ds1820 start convert
    DS18B20_Rst();
    DS18B20_Check();	 
    DS18B20_Write_Byte(0xcc);// skip rom   rt_thread_mdelay
    DS18B20_Write_Byte(0xbe);// convert	    
    TL=DS18B20_Read_Byte(); // LSB   
    TH=DS18B20_Read_Byte(); // MSB  
	    	  
    if(TH>7)
    {
        TH=~TH;
        TL=~TL; 
        temp=0;//�¶�Ϊ��  
    }else temp=1;//�¶�Ϊ��	  	  
    tem=TH; //��ø߰�λ
    tem<<=8;    
    tem+=TL;//��õװ�λ
    tem=tem*0.625;//ת��     
	if(temp)return tem; //�����¶�ֵ
	else return -tem;    
} 
 

int DS18B20_TEST(void)
{
    rt_uint8_t count;
		short temp=0;
    for(count = 1 ; count <= 3 ;count++){     
			temp =DS18B20_Get_Temp();  
      rt_kprintf("temperature : %d\r\n", temp);
      rt_thread_mdelay(500);
    }
    return 0;
}
MSH_CMD_EXPORT(DS18B20_TEST, temperature);

	
void temperature_get_val(void* parameter)
{
	rt_uint8_t i = 0;
  rt_uint16_t var[10] = {0};
	while(DS18B20_Init()){  rt_kprintf("ds18b20 error");}	 //DS18B20��ʼ��	
		
	while(1)
	{
		for(i = 0;i < 10;i++){	
				var[i] =DS18B20_Get_Temp();
				rt_thread_delay(1);             
		}
		Temperature[0] = Bubble_filter(var,0);  //ȡ��Сֵ

		Temperature[1] = Temperature[0]/10;  	 //����λ
		Temperature[2] = Temperature[0]%10;		 //С��λ

		rt_thread_delay(100);
	}
	
}

int ds18b20_thread_init()
{
	rt_thread_t tid;
        tid = rt_thread_create("Temperature",temperature_get_val, RT_NULL,1024,3, 20);
        if (tid != RT_NULL)
                rt_thread_startup(tid);
        
        return 0;

	
}
INIT_APP_EXPORT(ds18b20_thread_init);
