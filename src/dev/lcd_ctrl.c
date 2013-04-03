/**
 *******************************************************************************
 * @file    lcd_ctrl.c
 * @author  Olli Vanhoja
 * @brief   LCD control library.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

/** @addtogroup lcd
  * @{
  */

#include "kernel.h"
#include "stm32f0xx_conf.h"
#include "lcd_ctrl.h"

/* GPIO macros */
#define GPIO_HIGH(a,b)      a->BSRR = b
#define GPIO_LOW(a,b)		a->BRR = b
#define GPIO_TOGGLE(a,b)    a->ODR ^= b

/* GPIO Pin setting */
#define RS              GPIO_Pin_4
#define EN              GPIO_Pin_5
#define D4              GPIO_Pin_0
#define D5              GPIO_Pin_1
#define D6              GPIO_Pin_2
#define D7              GPIO_Pin_3

#define  RS_HIGH        GPIO_HIGH(GPIOC, RS)
#define  RS_LOW         GPIO_LOW(GPIOC, RS)
#define  RS_TOGGLE      GPIO_TOGGLE(GPIOC, RS)
#define  EN_HIGH        GPIO_HIGH(GPIOC, EN)
#define  EN_LOW         GPIO_LOW(GPIOC, EN)
#define  EN_TOGGLE      GPIO_TOGGLE(GPIOC, EN)
#define  D4_HIGH        GPIO_HIGH(GPIOC, D4)
#define  D4_LOW         GPIO_LOW(GPIOC, D4)
#define  D4_TOGGLE      GPIO_TOGGLE(GPIOC, D4)
#define  D5_HIGH        GPIO_HIGH(GPIOC, D5)
#define  D5_LOW         GPIO_LOW(GPIOC, D5)
#define  D5_TOGGLE      GPIO_TOGGLE(GPIOC, D5)
#define  D6_HIGH        GPIO_HIGH(GPIOC, D6)
#define  D6_LOW         GPIO_LOW(GPIOC, D6)
#define  D6_TOGGLE      GPIO_TOGGLE(GPIOC, D6)
#define  D7_HIGH        GPIO_HIGH(GPIOC, D7)
#define  D7_LOW         GPIO_LOW(GPIOC, D7)
#define  D7_TOGGLE      GPIO_TOGGLE(GPIOC, D7)

static void lcdc_write_char(char c);

void  lcdc_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

    /* Set RS and EN pins as output PUSH PULL pins */
    GPIO_InitStructure.GPIO_Pin =  RS | EN ;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Set data pins as output OPEN DRAIN pins */
    GPIO_InitStructure.GPIO_Pin =  D4 | D5 | D6 | D7;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    RS_LOW; /* Write control bytes */
    osDelay(15); /* power on delay */
    GPIOC->ODR |= 0x03;
    EN_HIGH;
    osDelay(1); /* TODO 10us */
    EN_LOW;
    osDelay(5);
    EN_HIGH;
    osDelay(1); /* TODO 10us */
    EN_LOW;
    osDelay(1); /* TODO 100us */
    EN_HIGH;
    osDelay(1); /* TODO 10us */
    EN_LOW;
    osDelay(1);
    GPIOC->ODR &= 0xFFF0;
    GPIOC->ODR |= 0x02;
    EN_HIGH;
    osDelay(1); /* TODO 10us */
    EN_LOW;
    //lcd_write_char(0x20);
    lcdc_write_char(0x08); // display off
    lcdc_write_char(0x01); //LCD clear
    lcdc_write_char(0x0F); // display on, blink curson on
    lcdc_write_char(0x06); // entry mode
    //lcd_write_char(0x01); //LCD clear
    osDelay(10);
}


static void lcdc_write_char(char c)
{
    GPIOC->ODR &= 0xFFF0;
    GPIOC->ODR |= (c >> 4) & 0x0F;
    EN_HIGH;
    osDelay(1); /* TODO 10us */
    EN_LOW;
    GPIOC->ODR &= 0xFFF0;
    GPIOC->ODR |= c & 0x0F;
    EN_HIGH;
    osDelay(1); /* TODO 10us */
    EN_LOW;
    osDelay(2);
}

void lcdc_write(const char * buff)
{
    int i = 0;

    RS_HIGH; /* Write data */

    while (buff[i] != 0) {
        lcdc_write_char(buff[i]);
        i++;
    }
}

void lcdc_data_write(uint8_t data)
{
    RS_HIGH; /* Write data */
    lcdc_write_char(data);
}

void lcdc_reg_write(uint8_t val)
{
    RS_LOW; /* Write Register */
    lcdc_write_char(val);
}

void lcdc_clear(void)
{
  lcdc_reg_write(0x01);
}

void lcdc_home(void) {
  lcdc_reg_write(0x02);
}

void lcdc_goto(char pos)
{
    lcdc_reg_write(0x80 + pos);
}

void lcdc_print(char pos, const char * str)
{
    lcdc_goto(pos);
    lcdc_write(str);
}

/**
  * @}
  */

/**
  * @}
  */