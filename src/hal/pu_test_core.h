/**
 *******************************************************************************
 * @file    pu_test_core.h
 * @author  Olli Vanhoja
 * @brief   HAL test core
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup Test
  * @{
  */

#pragma once
#ifndef PU_TEST_CORE_H
#define PU_TEST_CORE_H

#ifndef PU_TEST_BUILD
    #error pu_test_core.h should not be #included in production!
#endif

/* Exception return values */
#define HAND_RETURN         0xFFFFFFF1u /*!< Return to handler mode using the MSP. */
#define MAIN_RETURN         0xFFFFFFF9u /*!< Return to thread mode using the MSP. */
#define THREAD_RETURN       0xFFFFFFFDu /*!< Return to thread mode using the PSP. */

#define DEFAULT_PSR         0x21000000u

/* IAR & core specific functions */
typedef uint32_t __istate_t;
#define __get_interrupt_state() 0
#define __enable_interrupt() do { } while(0)
#define __disable_interrupt() do { } while(0)
#define __set_interrupt_state(state) do { state = state; } while(0)

/* Stack frame saved by the hardware */
typedef struct {
    uint32_t r;
} hw_stack_frame_t;

/* Stack frame save by the software */
typedef struct {
    uint32_t r;
} sw_stack_frame_t;


/**
  * Init hw stack frame
  */
inline void init_hw_stack_frame(/*@unused@*/ osThreadDef_t * thread_def, /*@unused@*/ void * argument, /*@unused@*/ uint32_t a_del_thread)
{
}

/**
  * Request immediate context switch
  */
inline void req_context_switch(void)
{
}

/**
  * Save the context on the PSP
  */
inline void save_context(void)
{
}

/**
  * Load the context from the PSP
  */
inline void load_context(void)
{
}

/**
  * Read the main stack pointer
  */
inline void * rd_stack_ptr(void)
{
    return NULL;
}

/**
  * Read the PSP so that it can be stored in the task table
  */
inline void * rd_thread_stack_ptr(void)
{
    return NULL;
}

/**
  * Write stack pointer of the currentthread to the PSP
  */
inline void wr_thread_stack_ptr(/*@unused@*/ void * ptr)
{
}

inline uint32_t syscall(/*@unused@*/ int type, /*@unused@*/ void * p)
{
    return 0;
}

#endif /* PU_TEST_CORE_H */

/**
  * @}
  */

/**
  * @}
  */
