/*
 * Cortex-M33 exception handlers.
 *
 * Owned elsewhere, do not define here:
 *   SVC_Handler, PendSV_Handler  -> ThreadX port (tx_thread_schedule.S)
 *   SysTick_Handler              -> Core/Startup/tx_initialize_low_level.S
 *   TIM6_IRQHandler              -> stm32h5xx_hal_timebase_tim.c (HAL tick)
 *
 * Peripheral IRQ handlers go at the bottom of this file.
 */
#include "main.h"

void NMI_Handler(void)
{
    for (;;)
    {
    }
}

/* Set a breakpoint here, then inspect SCB->CFSR / HFSR / MMFAR / BFAR. */
void HardFault_Handler(void)
{
    for (;;)
    {
    }
}

void MemManage_Handler(void)
{
    for (;;)
    {
    }
}

void BusFault_Handler(void)
{
    for (;;)
    {
    }
}

void UsageFault_Handler(void)
{
    for (;;)
    {
    }
}

void DebugMon_Handler(void)
{
}

/* ---- Peripheral interrupt handlers ---- */
