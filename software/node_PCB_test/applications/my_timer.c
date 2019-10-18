#include "my_init.h"
#include "board.h" 

/* ��ʱ���Ŀ��ƿ� */
static rt_timer_t timer1;
static rt_timer_t timer2;

extern rt_uint16_t WASHING_count;                             //��ʱ����ϴ������ֵ
extern rt_uint16_t HEAT_count;                                //��ʱ���ȣ�����ֵ
extern rt_uint16_t FEED_count;                                //��ʱιʳ������ֵ
extern rt_uint8_t STEER_START_RUN_FLAG;              //�����ʼ��ת��־λ,0��ʾֹͣ��ת��1��ʾ��ʼ��ת
extern bool          HEAT_START_FLAG;                   //��ʼ���Ƽ���˿���ȱ�־λ
extern bool          FEED_START_FLAG;                   //��ʼιʳ�����־λ
extern bool          WASHING_START_FLAG;                //��ʼ��ϴ�����־λ
extern rt_uint16_t Washing_Time;                      //��ϴʱ�䣬����λ������
extern rt_uint16_t Heat_Time;                         //����ʱ�䣬����λ����������λ��
extern rt_uint16_t Feed_Time;                         //ιʳʱ�䣬����λ����������λ��




void Timing_Count(void);


static void timeout1(void* parameter)// ��ʱ��1��ʱ����
{
   //rt_kprintf("periodic timer is timeout\n");

	turn_led();
	//Timing_Count(); //�趨�豸���̵�����MOS�ܣ���ʱ���� [��δ���ܵ�������Ķ�ʱ�ر�ָ��Լ��Ķ�ʱϵͳ������Ӧ���趨ʱ��ʱҲ�Զ��ر�]


}

static void timeout2(void* parameter)  
{

	buzzer_ring();//������

}





void Timing_Count(void) //��Ƭ���ڲ���ʱ��  ȷ�����������ߺ�Ҳ�ܶ�ʱ�ر� ����״̬  ��ʱ����
{
			if(WASHING_START_FLAG==TRUE)
		  {
				WASHING_count++;
			  if(WASHING_count>=(Washing_Time*10))               //��ϴʱ�䵽ֹͣ��ϴ����ʱ��
			   {
						WASHING_count=0;
						Washing_Time=0;                                //��ϴ��ɣ���ϴʱ����0
						WASHING_START_FLAG=FALSE;
					  //rt_kprintf("Water_OFF!\n");
		      }
	   	}
			else {
					rt_pin_write(Watering_PIN, PIN_LOW);//�ضϳ�ˮ���
			}
			
			if(HEAT_START_FLAG==TRUE)
			{
		    	HEAT_count++;
				  if(HEAT_count>=(Heat_Time*10))                 //����˿����ʱ�䵽ֹͣ���ȡ���ʱ��
					{
						HEAT_count=0;                                //����ʱ�����ֵ��0
						Heat_Time=0;                                 //����ʱ����0
						HEAT_START_FLAG=FALSE;                       //��ʼ���ȱ�־λ��0
						//rt_kprintf("Heat1_OFF!\n");rt_kprintf("Heat2_OFF!\n");
					}
			}
			else {
				  rt_pin_write(Heat1_PIN, PIN_LOW);//�ضϼ���MOS
			   	rt_pin_write(Heat2_PIN, PIN_LOW);
					rt_pin_write(LED3, PIN_LOW);
			}
			if(FEED_START_FLAG==TRUE)
			{
		    	FEED_count++;
				  if(FEED_count>=(Feed_Time*10))                 //ιʳ����ʱ�䵽ֹͣʱ�䡾��ʱ��
					{
						Feed_Time=0;                                 //ιʳʱ����0
					  FEED_count=0;                                //ιʳʱ�����ֵ��0
						FEED_START_FLAG=FALSE;                       //��ʼιʳ��־λ��0
					}
			}
			else {
					rt_pin_write(Feeding_PIN, PIN_LOW);//�ضϵ�ŷ�  
			}
}


int my_timer_init(void)
{
    /* ������ʱ��1 */
    timer1 = rt_timer_create("timer1",  /* ��ʱ�������� timer1 */
                        timeout1, /* ��ʱʱ�ص��Ĵ����� */
                        RT_NULL,  /* ��ʱ��������ڲ��� */
                        10,       /* ��ʱ���ȣ���OS TickΪ��λ����10��OS Tick   --> 10MS*/  
                        RT_TIMER_FLAG_PERIODIC); /* �����Զ�ʱ�� */
    /* ������ʱ�� */
    if (timer1 != RT_NULL) rt_timer_start(timer1);


	
	
	    /* ������ʱ��1 */
    timer2 = rt_timer_create("timer2",  /* ��ʱ�������� timer1 */
                        timeout2, /* ��ʱʱ�ص��Ĵ����� */
                        RT_NULL,  /* ��ʱ��������ڲ��� */
                        1,       /* ��ʱ���ȣ���OS TickΪ��λ����10��OS Tick   --> 10MS*/  
                        RT_TIMER_FLAG_PERIODIC); /* �����Զ�ʱ�� */
    /* ������ʱ�� */
    if (timer2 != RT_NULL) rt_timer_start(timer2);

    return 0;
}
INIT_APP_EXPORT(my_timer_init);

