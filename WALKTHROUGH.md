# Reading an analog value with ThreadX tasks

Target: NUCLEO-H563ZI. Input: **PA6 = ADC1_IN11 (Arduino A1, CN9)**. Wire a potentiometer
between 3V3 and GND with the wiper on A1 (or just touch A1 to 3V3 / GND to see it move).

Build and flash as in `README.md`. Then add `adc_latest_raw`, `adc_avg_raw`, `adc_avg_mv`
and `adc_dropped_samples` (all in `Core/Src/app_threadx.c`) to your debugger's live watch.

## The picture

```
 PA6 ──► ADC1 ──► adc_thread ──queue──► process_thread ──► yellow LED, adc_avg_*
                  (prio 8, 100 Hz)      (prio 9, blocks)
 heartbeat_thread (prio 11) toggles the green LED
```

Two files matter. Read them in this order.

## Step 1: the ADC itself (`Core/Src/adc_sensor.c`, no ThreadX in it)

`ADC_Sensor_Init()` runs in `main()` before the scheduler starts. It does five things:

1. **Kernel clock.** `RCC_PERIPHCLK_ADCDAC` is pointed at HCLK, then `__HAL_RCC_ADC_CLK_ENABLE()`.
2. **Pin to analog mode.** `GPIO_MODE_ANALOG`, no pull. A digital input buffer or pull resistor would corrupt the reading.
3. **ADC settings.** 12 bit, right-aligned, software trigger, single conversion, one channel. The ADC clock is HCLK/4 = 62.5 MHz.
4. **Calibration.** `HAL_ADCEx_Calibration_Start()`. Do it once, with the ADC off.
5. **Channel.** Channel 11 at rank 1 with a 47.5-cycle sampling time. A source with high impedance (more than a few kΩ) needs a longer sampling time or a buffer.

`ADC_Sensor_Read()` is the blocking read: `HAL_ADC_Start` → `HAL_ADC_PollForConversion` → `HAL_ADC_GetValue` → `HAL_ADC_Stop`. A conversion takes about 1 µs, so polling is fine.

Conversion to volts: `mV = raw * 3300 / 4095`. This assumes VREF+ is 3.3 V.

## Step 2: a periodic sampling thread (`adc_thread_entry`)

```c
next_wake += ADC_PERIOD_TICKS;
tx_thread_sleep(next_wake - now);
```

Sleeping a fixed 10 ticks after each read would drift, because the read itself takes time.
Sleeping until an absolute `tx_time_get()` value keeps the sample rate steady.
This is the thread's only job: sample, then hand off. It does no maths.

## Step 3: hand off with a queue (`tx_queue_send` / `tx_queue_receive`)

The sampler calls `tx_queue_send(..., TX_NO_WAIT)`. It must never stall on a slow consumer,
or the sample timing breaks. If the queue is full, the sample is dropped and
`adc_dropped_samples` goes up.

`process_thread` calls `tx_queue_receive(..., TX_WAIT_FOREVER)`. While the queue is empty it
uses no CPU. When a sample arrives, ThreadX wakes it. It averages 16 samples (~160 ms), writes
`adc_avg_raw` / `adc_avg_mv`, and sets the yellow LED when the average is above half scale.

Priorities: the sampler (8) outranks the processor (9), so sampling timing is never delayed by the
averaging. Lower number = higher priority in ThreadX.

## Things to try, in order

1. **Watch it work.** Turn the potentiometer; `adc_avg_mv` should follow from 0 to ~3300.
2. **Break the consumer.** Add `tx_thread_sleep(500)` inside `process_thread_entry`'s loop. `adc_dropped_samples` starts climbing. Make `SAMPLE_QUEUE_DEPTH` bigger and see it take longer to start dropping.
3. **Change the rate.** Set `ADC_PERIOD_TICKS` to 1 (1 kHz).
4. **Reduce noise.** Set `OversamplingMode = ENABLE` in `adc_sensor.c` (look at `ADC_OversamplingTypeDef`), or just raise `AVG_WINDOW`.
5. **Print the values.** Implement `__io_putchar()` (see `syscalls.c`) over USART3 (the ST-LINK virtual COM port, PD8/PD9), then `printf` from `process_thread`. Keep `printf` out of `adc_thread`; it is slow.
6. **Interrupt-driven read.** Use `HAL_ADC_Start_IT`, enable the ADC1 IRQ with a priority number of 5 or higher (ThreadX needs ISRs that call it to be at or below SysTick's 4; see the README's interrupt notes), and in `HAL_ADC_ConvCpltCallback` call `tx_semaphore_put`. The sampler then waits on the semaphore instead of polling.
7. **Timer-triggered + DMA.** Trigger the ADC from a timer (`ExternalTrigConv`) and use `HAL_ADC_Start_DMA` into a circular buffer. Signal the processing thread from the half/full-transfer callbacks. This is the right design for fast, gap-free sampling.
8. **Add channels.** Turn on `ScanConvMode`, set `NbrOfConversion`, and call `HAL_ADC_ConfigChannel` once per rank.

## Pitfalls

- **Calling HAL from two threads.** The ADC handle has no lock around it. If more than one thread reads the ADC, protect `ADC_Sensor_Read()` with a `TX_MUTEX`.
- **Stack size.** `TX_ENABLE_STACK_CHECKING` is on. If you add `printf`, raise `THREAD_STACK_SIZE` (newlib-nano printf needs a lot).
- **Pin.** If your board is not a Nucleo, change `ADC_INPUT_*` in `Core/Inc/board.h` and check the pin's ADC channel in the datasheet.
- **VREF.** `ADC_VREF_MV` is a constant. For absolute accuracy, read VREFINT and compute VDDA.
