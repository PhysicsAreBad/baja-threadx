/*
 * ADC walkthrough application. See WALKTHROUGH.md for the step-by-step explanation.
 *
 *   adc_thread (prio 8)      every 10 ms: read the ADC, post the sample to a queue
 *   process_thread (prio 9)  waits on the queue, averages 16 samples, drives the yellow LED
 *   heartbeat_thread (prio 11) green LED, shows the scheduler is alive
 *
 * All ThreadX objects are allocated from one byte pool. Timings are in ticks;
 * TX_TIMER_TICKS_PER_SECOND = 1000, so one tick is 1 ms.
 */
#include "tx_api.h"
#include "board.h"
#include "main.h"
#include "adc_sensor.h"

#define APP_BYTE_POOL_SIZE      (16 * 1024)
#define THREAD_STACK_SIZE       1024

#define ADC_THREAD_PRIO         8
#define PROCESS_THREAD_PRIO     9
#define HEARTBEAT_THREAD_PRIO   11

#define ADC_PERIOD_TICKS        10      /* 100 samples per second */
#define AVG_WINDOW              16      /* samples per averaged result */
#define LED_THRESHOLD_RAW       2048    /* yellow LED on above ~1.65 V */

/* A queue message is ULONGs; one ULONG per sample keeps it simple. */
#define SAMPLE_QUEUE_DEPTH      32

static UCHAR        app_byte_pool_mem[APP_BYTE_POOL_SIZE] __attribute__((aligned(8)));
static TX_BYTE_POOL app_byte_pool;

static TX_THREAD    adc_thread;
static TX_THREAD    process_thread;
static TX_THREAD    heartbeat_thread;
static TX_QUEUE     sample_queue;

/* Latest results. Add these to the debugger's Live Expressions window to watch them. */
volatile uint16_t   adc_latest_raw;
volatile uint32_t   adc_avg_raw;
volatile uint32_t   adc_avg_mv;
volatile uint32_t   adc_dropped_samples;

static void adc_thread_entry(ULONG input);
static void process_thread_entry(ULONG input);
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
    (void)first_unused_memory;

    if (tx_byte_pool_create(&app_byte_pool, "app pool", app_byte_pool_mem,
                            sizeof(app_byte_pool_mem)) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Queue size is in ULONGs (4 bytes): 1-ULONG messages, SAMPLE_QUEUE_DEPTH of them. */
    if (tx_queue_create(&sample_queue, "samples", 1,
                        app_alloc(SAMPLE_QUEUE_DEPTH * sizeof(ULONG)),
                        SAMPLE_QUEUE_DEPTH * sizeof(ULONG)) != TX_SUCCESS)
    {
        Error_Handler();
    }

    if (tx_thread_create(&adc_thread, "adc", adc_thread_entry, 0,
                         app_alloc(THREAD_STACK_SIZE), THREAD_STACK_SIZE,
                         ADC_THREAD_PRIO, ADC_THREAD_PRIO,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    if (tx_thread_create(&process_thread, "process", process_thread_entry, 0,
                         app_alloc(THREAD_STACK_SIZE), THREAD_STACK_SIZE,
                         PROCESS_THREAD_PRIO, PROCESS_THREAD_PRIO,
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

/* Producer: samples on a fixed period and hands each sample to the queue. */
static void adc_thread_entry(ULONG input)
{
    ULONG next_wake;

    (void)input;
    next_wake = tx_time_get();

    for (;;)
    {
        uint16_t raw;

        if (ADC_Sensor_Read(&raw) == 0)
        {
            adc_latest_raw = raw;
            /* TX_NO_WAIT: never block the sampler. If the consumer fell behind, count the loss. */
            if (tx_queue_send(&sample_queue, &(ULONG){raw}, TX_NO_WAIT) != TX_SUCCESS)
            {
                adc_dropped_samples++;
            }
        }

        /* Sleep until an absolute time so the period does not drift by the work done above. */
        next_wake += ADC_PERIOD_TICKS;
        ULONG now = tx_time_get();
        if ((LONG)(next_wake - now) > 0)
        {
            tx_thread_sleep(next_wake - now);
        }
        else
        {
            next_wake = now;    /* we overran the period; resynchronise */
        }
    }
}

/* Consumer: blocks until a sample arrives, averages AVG_WINDOW of them, updates the LED. */
static void process_thread_entry(ULONG input)
{
    ULONG sum = 0;
    ULONG count = 0;

    (void)input;

    for (;;)
    {
        ULONG sample;

        if (tx_queue_receive(&sample_queue, &sample, TX_WAIT_FOREVER) != TX_SUCCESS)
        {
            continue;
        }

        sum += sample;
        if (++count == AVG_WINDOW)
        {
            adc_avg_raw = sum / AVG_WINDOW;
            adc_avg_mv  = ADC_RawToMilliVolts(adc_avg_raw);
            HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN,
                              adc_avg_raw > LED_THRESHOLD_RAW ? GPIO_PIN_SET : GPIO_PIN_RESET);
            sum = 0;
            count = 0;
        }
    }
}

static void heartbeat_thread_entry(ULONG input)
{
    (void)input;

    for (;;)
    {
        HAL_GPIO_TogglePin(LED_GREEN_PORT, LED_GREEN_PIN);
        tx_thread_sleep(250);
    }
}
