#include "flash.h"

#define LOG_TAG             "flash"
#include "drv_log.h"

#include "drv_spi.h"
#include "spi_flash_sfud.h"
#include "easyflash.h"
#include "board.h"
#include "drv_hx711.h"
#include <stdio.h>
#include <partition.h>
/*---------------------- Constant / Macro Definitions -----------------------*/
#define FLASH_SPI_BUS_NAME             "spi2"
#define FLASH_SPI_DEV_NAME             "spi20"



/* SPI �豸���� */
rt_spi_flash_device_t nor_flash;




void nor_flash_init(void)
{
	 /* ��ʼ�� nor_flash Flash �豸 */
    rt_hw_spi_device_attach(FLASH_SPI_BUS_NAME, FLASH_SPI_DEV_NAME, GPIOB, GPIO_PIN_12);
	
	  nor_flash =  rt_sfud_flash_probe("nor_flash", "spi20");
	
	    /* ��ʼ�� nor_flash Flash �豸 */
    if ( nor_flash == RT_NULL ) {
        /* Flash ��ʼ��ʧ�� */
        LOG_E("ERROR: SPI device %s not found!\n", FLASH_SPI_DEV_NAME);
    }    
		
		/* ��ʼ�� Flash �ϵĸ��� ���� */
    extern int rt_partition_init(const char* flash_device, const struct rt_partition* parts, rt_size_t num);
    static const struct rt_partition partition[] = RT_PARTITION_DEFAULT_TABLE;
    rt_partition_init("nor_flash", partition, sizeof(partition) / sizeof(struct rt_partition));

    /* ��ʼ�� EasyFlash ģ�� */
		easyflash_init();
		flash_read_normalized();
}


int flash_read_normalized(void)
{
    char string[100];

    ef_port_read(FLASH_INIT_ADDRESS+0,(uint32_t *)&Reverse_Normalized[0],4);	    
    ef_port_read(FLASH_INIT_ADDRESS+4,(uint32_t *)&Reverse_Normalized[1],4);	

    ef_port_read(FLASH_INIT_ADDRESS+8 ,&var_min[0],4);	
    ef_port_read(FLASH_INIT_ADDRESS+12,&var_min[1],4);
    
    Normalized[0] = (double)1 / Reverse_Normalized[0] ;
    Normalized[1] = (double)1 / Reverse_Normalized[1]  ;
    
    //���ݴ����string�� 
    sprintf(string,"Normalized*100: %f  , %f ",Normalized[0]*100,Normalized[1]*100);
    LOG_I(string);
    
    sprintf(string,"var_min:%ld  , %ld ",var_min[0],var_min[1]);
    LOG_I(string);
    return 0;
}






