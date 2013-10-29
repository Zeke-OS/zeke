# Common to all
hal-SRC-1 += src/hal/uart.c

# Target model specific modules
ifeq ($(configMCU_MODEL),MCU_MODEL_STM32F0)
	# Includes
	IDIR += ./Libraries/CMSIS/Include ./Libraries/STM32F0xx/Drivers/inc
	IDIR += ./Libraries/STM32F0xx/CMSIS
	IDIR += ./Libraries/STM32F0xx/Drivers/inc
	# TODO not always Discovery
	IDIR += ./Libraries/Discovery
	# Soúrces
	hal-SRC-1 += $(wildcard Libraries/STM32F0xx/CMSIS/*.c)
	#hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_dbgmcu.c
	hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_exti.c
	hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_gpio.c
	hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_misc.c
	#hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_pwr.c
	hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_rcc.c
	hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_syscfg.c
	#hal-SRC-1 += Libraries/STM32F0xx/Drivers/src/stm32f0xx_tim.c
	# TODO not always Discovery
	hal-SRC-1 += $(wildcard Libraries/Discovery/*.c)
	hal-SRC-1 += $(wildcard src/hal/stm32f0/*.c)
endif
ifeq ($(configMCU_MODEL),MCU_MODEL_BCM2835)
	# NOTE: We don't want to include STARTUP here!
	hal-ASRC-1 += $(filter-out $(STARTUP), $(wildcard src/hal/bcm2835/*.S))
	hal-SRC-1 += $(wildcard src/hal/bcm2835/*.c)
endif

# Select HAL sources
hal-SRC-1 += src/hal/sysinfo.c
hal-SRC-$(configATAG) += src/hal/atag.c
hal-SRC-$(configMMU) += src/hal/mmu.c
ifeq ($(configARM_PROFILE_M),1)
	hal-SRC-1 += $(wildcard src/hal/cortex_m/*.c)
else
#	ifeq ($(configARCH),__ARM4T__) # ARM9
#	endif
	# ARM11
	ifneq (,$(filter $(configARCH),__ARM6__ __ARM6K__)) # Logical OR
		hal-ASRC-1 += $(wildcard src/hal/arm11/*.S)
		hal-SRC-1 += $(wildcard src/hal/arm11/*.c)
	endif
endif

