/*
 * ThreadX low-level initialization for STM32H563 (Cortex-M33, GNU).
 *
 * Adapted from third_party/threadx/ports/cortex_m33/gnu/src/tx_initialize_low_level.S.
 * Differences from the generic port file:
 *   - uses the ST startup vector table (g_pfnVectors)
 *   - SysTick reload is computed at runtime from SystemCoreClock, so it stays
 *     correct whatever clock SystemClock_Config() picks (call that first)
 *   - first free RAM comes from __tx_free_memory_start in the linker script
 *
 * SysTick is owned by ThreadX. The HAL tick (HAL_Delay/HAL_GetTick) runs on TIM6,
 * see Core/Src/stm32h5xx_hal_timebase_tim.c.
 */

#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif

#ifndef TX_TIMER_TICKS_PER_SECOND
#define TX_TIMER_TICKS_PER_SECOND 100
#endif

    .syntax unified
    .thumb

    .global _tx_thread_system_stack_ptr
    .global _tx_initialize_unused_memory
    .global _tx_timer_interrupt
    .global SystemCoreClock
    .global g_pfnVectors
    .global __tx_free_memory_start

/* VOID _tx_initialize_low_level(VOID) */
    .section .text._tx_initialize_low_level, "ax", %progbits
    .balign 4
    .global  _tx_initialize_low_level
    .thumb_func
    .type _tx_initialize_low_level, %function
_tx_initialize_low_level:
    /* Disable interrupts during ThreadX initialization. */
    CPSID   i

    /* Hand the first unused RAM address to tx_application_define(). */
    LDR     r0, =_tx_initialize_unused_memory
    LDR     r1, =__tx_free_memory_start
    STR     r1, [r0]

    /* Vector table offset register. */
    MOV     r0, #0xE000E000
    LDR     r1, =g_pfnVectors
    STR     r1, [r0, #0xD08]

    /* Enable the DWT cycle counter (handy for profiling). */
    LDR     r0, =0xE000EDFC                 // CoreDebug->DEMCR
    LDR     r1, [r0]
    ORR     r1, r1, #0x01000000             // TRCENA
    STR     r1, [r0]
    LDR     r0, =0xE0001000                 // DWT->CTRL
    LDR     r1, [r0]
    ORR     r1, r1, #1                      // CYCCNTENA
    STR     r1, [r0]

    /* ISRs run on the reset (main) stack. */
    LDR     r0, =_tx_thread_system_stack_ptr
    LDR     r1, =g_pfnVectors
    LDR     r1, [r1]
    STR     r1, [r0]

    /* SysTick: reload = SystemCoreClock / TX_TIMER_TICKS_PER_SECOND - 1, core clock. */
    MOV     r0, #0xE000E000
    LDR     r2, =SystemCoreClock
    LDR     r2, [r2]
    LDR     r3, =TX_TIMER_TICKS_PER_SECOND
    UDIV    r1, r2, r3
    SUB     r1, r1, #1
    STR     r1, [r0, #0x14]                 // SYST_RVR
    MOV     r1, #0
    STR     r1, [r0, #0x18]                 // SYST_CVR
    MOV     r1, #0x7                        // CLKSOURCE | TICKINT | ENABLE
    STR     r1, [r0, #0x10]                 // SYST_CSR

    /* System handler priorities (4 priority bits on STM32H5, upper nibble). */
    LDR     r1, =0x00000000                 // SecureFault, UsageFault, BusFault, MemManage
    STR     r1, [r0, #0xD18]
    LDR     r1, =0xFF000000                 // SVCall lowest
    STR     r1, [r0, #0xD1C]
    LDR     r1, =0x40FF0000                 // SysTick = 4, PendSV lowest, DebugMon = 0
    STR     r1, [r0, #0xD20]

    BX      lr
    .size _tx_initialize_low_level, . - _tx_initialize_low_level


/* ThreadX tick. */
    .section .text.SysTick_Handler, "ax", %progbits
    .balign 4
    .global  SysTick_Handler
    .thumb_func
    .type SysTick_Handler, %function
SysTick_Handler:
    PUSH    {r0, lr}
#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))
    BL      _tx_execution_isr_enter
#endif
    BL      _tx_timer_interrupt
#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))
    BL      _tx_execution_isr_exit
#endif
    POP     {r0, lr}
    BX      lr
    .size SysTick_Handler, . - SysTick_Handler

    .end
