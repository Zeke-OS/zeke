.cpu cortex-m0
.thumb

.extern HardFault_Handler
.extern SVC_Handler
.extern PendSV_Handler
.extern SysTic_Handler

    .global reset_handler
    .section .vectors, "ax"
    .code 16
    .align 0
    .global _vectors

.macro DEFAULT_ISR_HANDLER name=
    .thumb_func
    .weak \name
\name:
1:  b 1b /* inf loop */
.endm

_vectors:
    .word __stack_end__
    .word reset_handler         /*    Reset     */
    .word NMI_Handler           /*    NMI       */
    .word HardFault_Handler     /*    HardFault */
    .word 0                     /*    R         */
    .word 0                     /*    E         */
    .word 0                     /*    S         */
    .word 0                     /*    E         */
    .word 0                     /*    R         */
    .word 0                     /*    V         */
    .word 0                     /*    E         */
    .word SVC_Handler           /*    SVCall    */
    .word 0                     /*    reserved  */
    .word PendSV_Handler        /*    PendSV    */
    .word SysTick_Handler       /*    SysTick   */
    /* External Interrupts                      */
    .word WWDG_IRQHandler       /* 0  WWDG      */
    .word PVD_IRQHandler        /* 1  PVD       */
    .word RTC_IRQHandler        /* 2  RTC       */
    .word FLASH_IRQHandler      /* 3  FLASH     */
    .word RCC_IRQHandler        /* 4  RCC       */
    .word EXTI0_1_IRQHandler    /* 5  EXTI0_1   */
    .word EXTI2_3_IRQHandler    /* 6  EXTI2_3   */
    .word EXTI4_15_IRQHandler   /* 7  EXTI4_15  */
    .word TS_IRQHandler         /* 8  TSC       */
    .word DMA1_Channel1_IRQHandler   /* 9  DMA_CH1   */
    .word DMA1_Channel2_3_IRQHandler /* 10 DMA_CH2_3 */
    .word DMA1_Channel4_5_IRQHandler /* 11 DMA_CH4_5 */
    .word ADC1_COMP_IRQHandler  /* 12 ADC_COMP  */
    .word TIM1_BRK_UP_TRG_COM_IRQHandler /* 13 TIM1_BRK_UP_TRG_COM */
    .word TIM1_CC_IRQHandler    /* 14 TIM1_CC   */
    .word TIM2_IRQHandler       /* 15 TIM2      */
    .word TIM3_IRQHandler       /* 16 TIM3      */
    .word TIM6_DAC_IRQHandler   /* 17 TIM6_DAC  */
    .word 0                     /* 18 reserved  */
    .word TIM14_IRQHandler      /* 19 TIM14     */
    .word TIM15_IRQHandler      /* 20 TIM15     */
    .word TIM16_IRQHandler      /* 21 TIM16     */
    .word TIM17_IRQHandler      /* 22 TIM17     */
    .word I2C1_IRQHandler       /* 23 I2C1      */
    .word I2C2_IRQHandler       /* 24 I2C2      */
    .word SPI1_IRQHandler       /* 25 SPI1      */
    .word SPI2_IRQHandler       /* 26 SPI2      */
    .word USART1_IRQHandler     /* 27 USART1    */
    .word USART2_IRQHandler     /* 28 USART2    */
    .word 0                     /* 29 reserved  */
    .word CEC_IRQHandler        /* 30 CEC       */
    .word 0                     /*    reserved  */

    .section .init, "ax"
    .thumb_func

    .reset_handler:
    b _start

DEFAULT_ISR_HANDLER NMI_Handler
;DEFAULT_ISR_HANDLER HardFault_Handler
;DEFAULT_ISR_HANDLER SVC_Handler
;DEFAULT_ISR_HANDLER PendSV_Handler
;DEFAULT_ISR_HANDLER SysTick_Handler
DEFAULT_ISR_HANDLER WWDG_IRQHandler
DEFAULT_ISR_HANDLER PVD_IRQHandler
DEFAULT_ISR_HANDLER RTC_IRQHandler
DEFAULT_ISR_HANDLER FLASH_IRQHandler
DEFAULT_ISR_HANDLER RCC_IRQHandler
DEFAULT_ISR_HANDLER EXTI0_1_IRQHandler
DEFAULT_ISR_HANDLER EXTI2_3_IRQHandler
DEFAULT_ISR_HANDLER EXTI4_15_IRQHandler
DEFAULT_ISR_HANDLER TS_IRQHandler
DEFAULT_ISR_HANDLER DMA1_Channel1_IRQHandler
DEFAULT_ISR_HANDLER DMA1_Channel2_3_IRQHandler
DEFAULT_ISR_HANDLER DMA1_Channel4_5_IRQHandler
DEFAULT_ISR_HANDLER ADC1_COMP_IRQHandler
DEFAULT_ISR_HANDLER TIM1_BRK_UP_TRG_COM_IRQHandler
DEFAULT_ISR_HANDLER TIM1_CC_IRQHandler
DEFAULT_ISR_HANDLER TIM2_IRQHandler
DEFAULT_ISR_HANDLER TIM3_IRQHandler
DEFAULT_ISR_HANDLER TIM6_DAC_IRQHandler
DEFAULT_ISR_HANDLER TIM14_IRQHandler
DEFAULT_ISR_HANDLER TIM15_IRQHandler
DEFAULT_ISR_HANDLER TIM16_IRQHandler
DEFAULT_ISR_HANDLER TIM17_IRQHandler
DEFAULT_ISR_HANDLER I2C1_IRQHandler
DEFAULT_ISR_HANDLER I2C2_IRQHandler
DEFAULT_ISR_HANDLER SPI1_IRQHandler
DEFAULT_ISR_HANDLER SPI2_IRQHandler
DEFAULT_ISR_HANDLER USART1_IRQHandler
DEFAULT_ISR_HANDLER USART2_IRQHandler
DEFAULT_ISR_HANDLER CEC_IRQHandler
