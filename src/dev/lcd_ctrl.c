/**
 *******************************************************************************
 * @file    lcd_ctrl.c
 * @author  Olli Vanhoja
 * @brief   Control library for 162B series LCD.
 *
 *          This part of the LCD control library is purely run in thread scope.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

/** @addtogroup lcd
  * @{
  */

#include "kernel.h"
#include "queue.h"
#include "stm32f0xx_conf.h"
#include "lcd_ctrl.h"

/* GPIO macros */
#define GPIO_HIGH(a,b)      a->BSRR = b /*!< Set GPIO state to 1 */
#define GPIO_LOW(a,b)		a->BRR = b  /*!< Set GPIO state to 0 */
#define GPIO_TOGGLE(a,b)    a->ODR ^= b /*!< Toggle GPIO state. */

/* GPIO Pin setting */
#define RS              GPIO_Pin_4  /*!< RS pin position */
#define EN              GPIO_Pin_5  /*!< EN pin position */
#define D4              GPIO_Pin_0  /*!< D4 pin position */
#define D5              GPIO_Pin_1  /*!< D5 pin position */
#define D6              GPIO_Pin_2  /*!< D6 pin position */
#define D7              GPIO_Pin_3  /*!< D7 pin position */

#define  RS_HIGH        GPIO_HIGH(GPIOC, RS)    /*!< Set RS to 1 */
#define  RS_LOW         GPIO_LOW(GPIOC, RS)     /*!< Set RS to 0 */
#define  RS_TOGGLE      GPIO_TOGGLE(GPIOC, RS)  /*!< Toggle RS pin */
#define  EN_HIGH        GPIO_HIGH(GPIOC, EN)    /*!< Set EN to 1 */
#define  EN_LOW         GPIO_LOW(GPIOC, EN)     /*!< Set EN to 0 */
#define  EN_TOGGLE      GPIO_TOGGLE(GPIOC, EN)  /*!< Toggle EN pin */

static char lcdc_thread_stack[500];
/**
 * Buffer that is used between dev driver thread and syscall handler in kernel.
 */
char lcdc_buff[80];
queue_cb_t lcdc_queue_cb; /*!< Control block for LCD buffer queue. */

static void lcdc_init_lcd(void);
void lcdc_thread(void const * arg);
static void lcdc_tab(void);
static void lcdc_write_char(char c);
static void lcdc_write(const char * c);
static void lcdc_data_write(uint8_t data);
static void lcdc_reg_write(uint8_t val);
static void lcdc_clear(void);
static void lcdc_home(void);
static void lcdc_goto(char pos);

/**
 * Initialize LCD control driver
 */
osThreadId lcdc_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

    /* Set RS and EN pins as output PUSH PULL pins */
    GPIO_InitStructure.GPIO_Pin = RS | EN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Set data pins as output OPEN DRAIN pins */
    GPIO_InitStructure.GPIO_Pin = D4 | D5 | D6 | D7;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    lcdc_queue_cb = queue_create(lcdc_buff, sizeof(char), sizeof(lcdc_buff) / sizeof(char));

    osThreadDef_t th = {
        .pthread   = (os_pthread)(&lcdc_thread),
        .tpriority = osPriorityBelowNormal,
        .stackAddr = lcdc_thread_stack,
        .stackSize = sizeof(lcdc_thread_stack) / sizeof(char)
    };

    return osThreadCreate(&th, NULL);
}

static void lcdc_init_lcd(void)
{
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
    lcdc_write_char(0x08); /* display off */
    lcdc_write_char(0x01); /* LCD clear */
    lcdc_write_char(0x0F); /* display on, blink curson on */
    lcdc_write_char(0x06); /* entry mode */
    //lcd_write_char(0x01); /*LCD clear
    osDelay(10);
}

/**
 * Actual LCD driver thread code
 */
void lcdc_thread(void const * arg)
{
    /* Note: Prefer syscalls whenever possible */
    char ch;

    lcdc_init_lcd();

    while(1) {
        osWait(osWaitForever);
        while(queue_pop(&lcdc_queue_cb, &ch)) {
            /* Was it a control character? */
            if (ch <= 0x1f ||ch == 0x7f) {
                switch (ch) {
                case 0x02: /*Start of text */
                    lcdc_clear();
                case 0x08: /* BS (Backspace) */
                    lcdc_reg_write(0x14);
                    lcdc_data_write(' ');
                    lcdc_reg_write(0x14);
                    break;
                case 0x09: /* TAB (Horizontal tab) */
                    lcdc_tab();
                    break;
                case 0x0a: /* LF/NL (Line feed/New line) */
                    lcdc_goto(0x40); /** TODO This is incorrect temp code */
                    break;
                case 0x0d: /* CR (Carriage return) */
                    lcdc_home();
                    break;
                default:
                    break;
                }
            } else {
                lcdc_data_write((uint8_t)ch);
            }
        }
    }
}

/**
 * Write string of characters to the LCD.
 * @param buff
 */
static void lcdc_write(const char * buff)
{
    int i = 0;

    RS_HIGH; /* Write data */

    while (buff[i] != 0) {
        lcdc_write_char(buff[i]);
        i++;
    }
}

/**
 * Write data character to the LCD.
 */
static void lcdc_data_write(uint8_t data)
{
    RS_HIGH; /* Write data */
    lcdc_write_char(data);
}

/**
 * Write command word to the LCD.
 */
static void lcdc_reg_write(uint8_t val)
{
    RS_LOW; /* Write Register */
    lcdc_write_char(val);
}

/**
 * Clear the LCD.
 */
static void lcdc_clear(void)
{
  lcdc_reg_write(0x01);
}

/**
 * Set the LCD cursor position to home.
 */
static void lcdc_home(void)
{
  lcdc_reg_write(0x02);
}

/**
 * Write tab character to the LCD.
 */
static void lcdc_tab(void)
{
    RS_LOW; /* Write Register */
    lcdc_write_char(0x10);
    lcdc_write_char(0x10);
    lcdc_write_char(0x10);
    lcdc_write_char(0x10);
}

/**
 * Move LCD cursor to the position given in pos.
 * @param pos position.
 */
static void lcdc_goto(char pos)
{
    lcdc_reg_write(0x80 + pos);
}

/**
 * Write character to the LCD.
 */
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

/**
  * @}
  */

/**
  * @}
  */
