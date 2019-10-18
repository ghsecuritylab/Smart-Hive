//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//�о�԰����
//���̵�ַ��http://shop73023976.taobao.com/?spm=2013.1.0.0.M4PqC2
//
//  �� �� ��   : main.c
//  �� �� ��   : v2.0
//  ��    ��   : HuangKai
//  ��������   : 2014-0101
//  ����޸�   : 
//  ��������   : OLED 4�ӿ���ʾ����(51ϵ��)
//              ˵��: 
//              ----------------------------------------------------------------
//              GND    ��Դ��
//              VCC  ��5V��3.3v��Դ
//              D0   ��PD6��SCL��
//              D1   ��PD7��SDA��
//              RES  ��PD4
//              DC   ��PD5
//              CS   ��PD3               
//              ----------------------------------------------------------------
// �޸���ʷ   :
// ��    ��   : 
// ��    ��   : HuangKai
// �޸�����   : �����ļ�
//��Ȩ���У�����ؾ���
//Copyright(C) �о�԰����2014/3/16
//All rights reserved
//******************************************************************************/
#ifndef __OLED_H
#define __OLED_H			  	 
#include "sys.h"
#include "stdlib.h"	 

#define u8   unsigned char
#define u16  unsigned int
#define u32  unsigned long
//OLEDģʽ����
//0:4�ߴ���ģʽ
//1:����8080ģʽ
#define OLED_MODE 0
#define SIZE 16
#define XLevelL			0x00
#define XLevelH			0x10
#define Max_Column	128
#define Max_Row			64
#define	Brightness	0xFF 
#define X_WIDTH 		128
#define Y_WIDTH 		64	    						  

//-----------------OLED�˿ڶ���----------------  				
#define OLED_SCLK   53  // C12   D0
#define OLED_SDIN   52  // C11   D1
#define OLED_RST    51  // C10   RES
#define OLED_DC     50  // A15   DC

#define OLED_CS     44


//ʹ��4�ߴ��нӿ�ʱʹ�� 

#define OLED_SCLK_Clr() rt_pin_write(OLED_SCLK, PIN_LOW)//CLK
#define OLED_SCLK_Set() rt_pin_write(OLED_SCLK, PIN_HIGH)

#define OLED_SDIN_Clr() rt_pin_write(OLED_SDIN, PIN_LOW)//DIN
#define OLED_SDIN_Set() rt_pin_write(OLED_SDIN, PIN_HIGH)


#define OLED_RD_Clr() rt_pin_write(OLED_SCLK, PIN_LOW)  //D0 ʱ��
#define OLED_RD_Set() rt_pin_write(OLED_SCLK, PIN_HIGH)

#define OLED_WR_Clr() rt_pin_write(OLED_SDIN, PIN_LOW)  //D1 ����
#define OLED_WR_Set() rt_pin_write(OLED_SDIN, PIN_HIGH)

#define OLED_RST_Clr() rt_pin_write(OLED_RST, PIN_LOW)//RES
#define OLED_RST_Set() rt_pin_write(OLED_RST, PIN_HIGH)

#define OLED_DC_Clr() rt_pin_write(OLED_DC, PIN_LOW)   //DC
#define OLED_DC_Set() rt_pin_write(OLED_DC, PIN_HIGH)

#define OLED_CS_Clr()  rt_pin_write(OLED_CS, PIN_LOW)  //CS
#define OLED_CS_Set()  rt_pin_write(OLED_CS, PIN_HIGH)





//PC0~7,��Ϊ������
//#define DATAOUT(x) GPIO_Write(GPIOC,x);//���  



 		     
#define OLED_CMD  0	//д����
#define OLED_DATA 1	//д����



//OLED�����ú���
void OLED_WR_Byte(u8 dat,u8 cmd);	    
void OLED_Display_On(void);
void OLED_Display_Off(void);	   							   		    
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
void OLED_ShowChar(u8 x,u8 y,u8 chr);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);
void OLED_ShowString(u8 x,u8 y, u8 *p);	 
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHinese(u8 x,u8 y,u8 no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
#endif  
	 



