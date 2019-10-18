#include "my_init.h"

//---------------User Variable �û�����--------------------//


fx_date fx_data1={0};
bool   HEAT_START_FLAG=FALSE;          //��ʼ���Ƽ���˿���ȱ�־λ
bool   WASHING_START_FLAG=FALSE;       //��ʼ��ϴ�����־λ
bool   FEED_START_FLAG=FALSE;          //��ʼιʳ�����־λ

char *FW_status_string[4]={"ZhongWang","WangTai","ShiWang","ChuNuWang"}; //����״̬�� �ַ���

rt_uint32_t Count = 0;

rt_uint8_t FX_NUMBER = 0;
rt_uint8_t FW_status_flag;  //д��FLASH�Ĳ���  ��ʼ��������ֵ��ֱ�Ӷ�ȡFLASH��ֵ
rt_uint8_t PiShu;

rt_int32_t Weight[3]={0};
rt_uint16_t voltage[3] ={0};
rt_uint16_t Temperature[3] = {0};
rt_uint16_t W25_falsh_ID = 0;

rt_uint32_t Flow_Count = 0;



rt_uint8_t buzzer_time=0;//������
rt_uint8_t buzzer_count=0;//������
rt_uint8_t buzzer_number=0;//������


rt_uint8_t TEXT_Buffer[2]={0x04};    //��һ���ֽ�Ϊ����Ŀ���״̬��0x04��ʾ�������0x02���У�0x01��С

rt_uint8_t DataBuffer[8];   //���������ݴ����飬����2�ֽڵ�����+2�ֽڵ���������+2�ֽڵ��¶�+1�ֽڵ�(��ϴ�����ȡ�ιʳ)��־λ+1�ֽڵķ���״̬
rt_uint8_t send_buffer[(MAX_DATA_LENS)]; //����1�������ݻ������,
rt_uint8_t recv_buffer[MAX_DATA_LENS]; //WIFI ����2�������ݻ������,
rt_uint8_t recv_data_p=0x00;              //WIFI   ����2��������ָ��
rt_uint8_t recv_data_ok=0;                //���յ���ȷ�����ݰ�
rt_uint8_t sum_check=0;

rt_uint16_t WASHING_count;                             //��ʱ����ϴ������ֵ
rt_uint16_t HEAT_count;                                //��ʱ���ȣ�����ֵ
rt_uint16_t FEED_count;                                //��ʱιʳ������ֵ

rt_uint16_t Washing_Time=0;                 //��ϴʱ�䣬����λ������
rt_uint16_t Heat_Time=0;                    //����ʱ�䣬����λ����������λ��
rt_uint16_t Feed_Time=0;                    //ιʳʱ�䣬����λ����������λ��



rt_uint8_t Init_flag=0;
rt_uint8_t first_ON_OFF = 0;
rt_uint16_t Test_count = 0;

rt_int32_t Init_Value[2]={0};
rt_int32_t AD_Value[2]={0};
rt_int32_t Last_AD_Value[2]={0};
rt_int32_t side_weight[2]={0};


rt_int32_t temp[2]={0};
rt_int32_t var_max[2]={0};    //8632784        //�����ֵ�� ��Ϊ��һ�����ֵ
rt_int32_t var_min[2]={99999999,99999999};  //�����ֵС ��Ϊ��һ����Сֵ  -2147483648��+2141483647
rt_int32_t var0[10]={0};
rt_int32_t var1[10]={0};


rt_uint8_t set1kg_flag = 0;
float Normalized[2]={0};













