/*
 * Application threads. tx_application_define() is called by tx_kernel_enter()
 * before the scheduler starts: create threads, queues, mutexes, etc. here.
 *
 * All ThreadX objects are allocated from one byte pool so stack sizes can be
 * tuned in one place. Timings are in ticks; TX_TIMER_TICKS_PER_SECOND = 1000,
 * so one tick is 1 ms.
 */
#include "tx_api.h"
#include "board.h"
#include "main.h"

#define APP_BYTE_POOL_SIZE      (16 * 1024)
#define THREAD_STACK_SIZE       1024

#define BLINK_THREAD_PRIO       10
#define HEARTBEAT_THREAD_PRIO   11

static UCHAR        app_byte_pool_mem[APP_BYTE_POOL_SIZE] __attribute__((aligned(8)));
static TX_BYTE_POOL app_byte_pool;

static TX_THREAD    blink_thread;
static TX_THREAD    heartbeat_thread;
static TX_SEMAPHORE blink_sem;

static void blink_thread_entry(ULONG input);
static void heartbeat_thread_entry(ULONG input);

static VOID *app_alloc(ULONG size)
{
    VOID *ptr = TX_NULL;

    if (tx_byte_allocate(&app_byte_pool, &ptr, size, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    return ptr;
}

VOID tx_application_define(VOID *first_unused_memory)
{
    /* Free RAM beyond this point (up to the main stack) is also available if needed. */
    (void)first_unused_memory;

    if (tx_byte_pool_create(&app_byte_pool, "app pool", app_byte_pool_mem,
                            sizeof(app_byte_pool_mem)) != TX_SUCCESS)
    {
        Error_Handler();
    }

    if (tx_semaphore_create(&blink_sem, "blink sem", 0) != TX_SUCCESS)
    {
        Error_Handler();
    }

    if (tx_thread_create(&blink_thread, "blink", blink_thread_entry, 0,
                         app_alloc(THREAD_STACK_SIZE), THREAD_STACK_SIZE,
                         BLINK_THREAD_PRIO, BLINK_THREAD_PRIO,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    if (tx_thread_create(&heartbeat_thread, "heartbeat", heartbeat_thread_entry, 0,
                         app_alloc(THREAD_STACK_SIZE), THREAD_STACK_SIZE,
                         HEARTBEAT_THREAD_PRIO, HEARTBEAT_THREAD_PRIO,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }
}

/* Toggles the yellow LED each time the heartbeat thread signals it. */
static void blink_thread_entry(ULONG input)
{
    (void)input;

    for (;;)
    {
        tx_semaphore_get(&blink_sem, TX_WAIT_FOREVER);
        HAL_GPIO_TogglePin(LED_YELLOW_PORT, LED_YELLOW_PIN);
    }
}

/* Toggles the green LED every 250 ms and pings the blink thread every second. */
static void heartbeat_thread_entry(ULONG input)
{
    ULONG count = 0;

    (void)input;

    for (;;)
    {
        HAL_GPIO_TogglePin(LED_GREEN_PORT, LED_GREEN_PIN);
        if (++count % 4 == 0)
        {
            tx_semaphore_put(&blink_sem);
        }
        tx_thread_sleep(250);
    }
}
