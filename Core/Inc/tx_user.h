/*
 * ThreadX user configuration.
 *
 * Included by both C and assembly sources (TX_INCLUDE_USER_DEFINE_FILE),
 * so keep this file to plain #defines only.
 *
 * See third_party/threadx/common/inc/tx_user_sample.h for every option.
 */
#ifndef TX_USER_H
#define TX_USER_H

/* 1 ms kernel tick. tx_thread_sleep() etc. take ticks, so 1 tick == 1 ms. */
#define TX_TIMER_TICKS_PER_SECOND       1000

/* Highest priority is 0; 32 levels is plenty and keeps the ready list small. */
#define TX_MAX_PRIORITIES               32

/* Timer thread (runs application timer callbacks). */
#define TX_TIMER_THREAD_STACK_SIZE      1024
#define TX_TIMER_THREAD_PRIORITY        0

/* Fill new thread stacks with 0xEF and check them on context switch.
   Costs a little speed; handy while developing. Comment out for release. */
#define TX_ENABLE_STACK_CHECKING

/* Uncomment for release builds to skip API argument checking. */
/* #define TX_DISABLE_ERROR_CHECKING */

#endif /* TX_USER_H */
