#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "my_init.h"

extern rt_uint16_t voltage[3];

#define AD_CHANNEL 10






#define ADC_CHANNEL_MAX		(16) /* 0-15 */
#define __STM32_ADC(index, gpio, gpio_index) {index, GPIO##gpio##_CLK_ENABLE, GPIO##gpio, GPIO_PIN_##gpio_index}

static void GPIOA_CLK_ENABLE(void)
{
#ifdef __HAL_RCC_GPIOA_CLK_ENABLE
    __HAL_RCC_GPIOA_CLK_ENABLE();
#endif
}
static void GPIOB_CLK_ENABLE(void)
{
#ifdef __HAL_RCC_GPIOB_CLK_ENABLE
    __HAL_RCC_GPIOB_CLK_ENABLE();
#endif
}
static void GPIOC_CLK_ENABLE(void)
{
#ifdef __HAL_RCC_GPIOC_CLK_ENABLE
    __HAL_RCC_GPIOC_CLK_ENABLE();
#endif
}

struct adc_channel_index
{
    int index;
    void (*rcc)(void);
    GPIO_TypeDef *gpio;
    uint32_t pin;
};

static const struct adc_channel_index adc1_channels[] =
{
	__STM32_ADC(0, A, 0),
	__STM32_ADC(1, A, 1),
	__STM32_ADC(2, A, 2),
	__STM32_ADC(3, A, 3),
	__STM32_ADC(4, A, 4),
	__STM32_ADC(5, A, 5),
	__STM32_ADC(6, A, 6),
	__STM32_ADC(7, A, 7),
//	__STM32_ADC(8, B, 0),  //PWM
	__STM32_ADC(9, B, 1),

//	__STM32_ADC(11, C, 1), //HEAT2
//	__STM32_ADC(12, C, 2),
	

	__STM32_ADC(13, C, 3),
	__STM32_ADC(10, C, 0), //ADC
	__STM32_ADC(14, C, 4),
	__STM32_ADC(15, C, 5),
};


ADC_HandleTypeDef ADC1_Handler;	//ADC���

//��ʼ��ADC
//ch: ADC_channels 
//ͨ��ֵ 0~16ȡֵ��ΧΪ��ADC_CHANNEL_0~ADC_CHANNEL_16
void ADC_Init(ADC_HandleTypeDef ADCx_Handler)
{ 
    ADCx_Handler.Instance=ADCx_Handler.Instance;
    ADCx_Handler.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;   //4��Ƶ��ADCCLK=PCLK2/4=90/4=22.5MHZ
    ADCx_Handler.Init.Resolution=ADC_RESOLUTION_12B;              //8λģʽ
    ADCx_Handler.Init.DataAlign=ADC_DATAALIGN_RIGHT;             //�Ҷ���
    ADCx_Handler.Init.ScanConvMode=DISABLE;                      //��ɨ��ģʽ
    ADCx_Handler.Init.EOCSelection=DISABLE;                      //�ر�EOC�ж�
    ADCx_Handler.Init.ContinuousConvMode=DISABLE;                //�ر�����ת��
    ADCx_Handler.Init.NbrOfConversion=1;                         //1��ת���ڹ��������� Ҳ����ֻת����������1 
    ADCx_Handler.Init.DiscontinuousConvMode=DISABLE;             //��ֹ����������ģʽ
    ADCx_Handler.Init.NbrOfDiscConversion=0;                     //����������ͨ����Ϊ0
    ADCx_Handler.Init.ExternalTrigConv=ADC_SOFTWARE_START;       //�������
    ADCx_Handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;	//ʹ���������
    ADCx_Handler.Init.DMAContinuousRequests=DISABLE;             //�ر�DMA����
    HAL_ADC_Init(&ADCx_Handler);                       //��ʼ�� 
}

//ADC�ײ��������������ã�ʱ��ʹ��
//�˺����ᱻHAL_ADC_Init()����
//hadc:ADC���
void _ADC_Init(rt_uint8_t ch)
{
    GPIO_InitTypeDef GPIO_Initure;
    const struct adc_channel_index *index;
	
	
    __HAL_RCC_ADC1_CLK_ENABLE();            //ʹ��ADC1ʱ��
    adc1_channels[ch].rcc();			//����GPIOXʱ��
	
    GPIO_Initure.Pin = adc1_channels[ch].pin;            //PA4
    GPIO_Initure.Mode = GPIO_MODE_ANALOG;     //ģ��
    GPIO_Initure.Pull = GPIO_NOPULL;          //����������
    HAL_GPIO_Init(adc1_channels[ch].gpio,&GPIO_Initure);
}

rt_uint16_t Get_Adc(rt_uint8_t ch)   
{
    ADC_ChannelConfTypeDef ADC1_ChanConf;
    
    ADC1_ChanConf.Channel=ch;                                   //ͨ��
    ADC1_ChanConf.Rank=1;                                       //��1�����У�����1
    ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;        //����ʱ��
    ADC1_ChanConf.Offset=0;                 
    HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);        //ͨ������
	
    HAL_ADC_Start(&ADC1_Handler);                               //����ADC
	
    HAL_ADC_PollForConversion(&ADC1_Handler,10);                //��ѯת��
 
	return (rt_uint16_t)HAL_ADC_GetValue(&ADC1_Handler);	    //�������һ��ADC1�������ת�����
}

rt_err_t convert(struct rt_device *device, int channel, rt_uint16_t *value)
{
	static int old_channel = 0xFF;
	rt_err_t result = RT_EOK;
//	rt_uint16_t temp;

	if(channel > (ADC_CHANNEL_MAX - 1))
	{
		result = -RT_EIO;
		goto _exit;
	}
	if(old_channel != channel)
	{
		_ADC_Init(channel);
		old_channel = channel;
	}
//	temp = Get_Adc(channel);
	*value = Get_Adc(channel); 

_exit:
	return result;
}

static const struct rt_adc_ops adc_ops =
{
	convert,
};


int rt_hw_adc_init(void)
{
    int ret = RT_EOK;
	ADC1_Handler.Instance = ADC1;
	
	ADC_Init(ADC1_Handler);
	
	
	/* add ADC initial. */ 

    ret = rt_device_adc_create("adc1", &adc_ops, RT_NULL);

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_adc_init);




static rt_size_t _adc_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	rt_err_t result = RT_EOK;
	rt_size_t i;
    struct rt_device_adc *adc = (struct rt_device_adc *)dev;
	rt_uint16_t *value = (rt_uint16_t *)buffer;
	
	if(!adc->ops->convert)
	{
		return 0;
	}

	for(i=0; i<size; i+=sizeof(rt_uint16_t)/2)
	{
		result = adc->ops->convert(dev, pos + i, value);
		if(result != RT_EOK)
		{
			return 0;
		}
		value++;
	}

    return i;
}

rt_err_t rt_device_adc_register(struct rt_device_adc *device, const char *name, const struct rt_adc_ops *ops, const void *user_data)
{
	rt_err_t result = RT_EOK;
	
	memset(device, 0, sizeof(struct rt_device_adc));

    device->parent.type         = RT_Device_Class_Miscellaneous;

    device->parent.init         = RT_NULL;
    device->parent.open         = RT_NULL;
    device->parent.close        = RT_NULL;
    device->parent.read         = _adc_read;
    device->parent.write        = RT_NULL;
    device->parent.control      = RT_NULL;

    device->ops                 = ops;
    device->parent.user_data    = (void *)user_data;

    result = rt_device_register(&device->parent, name, RT_DEVICE_FLAG_RDWR);

    return result;
}

rt_err_t rt_device_adc_create(const char *name, const struct rt_adc_ops *ops, const void *user_data)
{
	rt_err_t result = RT_EOK;
	struct rt_device_adc *device;
	
	device = rt_malloc(sizeof(struct rt_device_adc));
	if(!device)
	{
		result = -RT_ENOMEM;
		goto _exit;
	}
	
	result = rt_device_adc_register(device, name, ops, user_data);
	
_exit:	
    return result;
}





void voltage_get_val(void* parameter)
{
		rt_device_t dev;
		rt_uint8_t i;
		rt_uint16_t temp = 0;
		rt_uint16_t var[10] = {0};

		
		dev = rt_device_find("adc1");
		rt_device_open(dev, RT_DEVICE_FLAG_RDONLY);
		while(1){
			for(i = 0;i < 10;i++){
				if( rt_device_read(dev, AD_CHANNEL, &temp, sizeof(temp) ) != sizeof(temp) ){  //12λ����
						rt_kprintf("channel %d: faild! \n", AD_CHANNEL);
				}
						var[i] = temp;
						rt_thread_delay(1);             
			}
			temp = Bubble_filter(var,1); //��ֵ�˲�

			voltage[0] = (temp*100+44800)/227;  //�������
			voltage[1] = voltage[0]/100;  //����λ
			voltage[2] = voltage[0]%100;  //С��λ
			
			rt_thread_delay(10);
		}
        
}

int voltage_collect(void)
{
    rt_thread_t tid;
        tid = rt_thread_create("Voltage_Collect",voltage_get_val, RT_NULL,1024,4, 20);
        if (tid != RT_NULL)
                rt_thread_startup(tid);
        
        return 0;
}
INIT_APP_EXPORT(voltage_collect);

