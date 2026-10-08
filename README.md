# STM32H563 + ThreadX: reading an analog input (walkthrough)

Copy of the base project with an ADC example added. **Start with [WALKTHROUGH.md](WALKTHROUGH.md).**
The rest of this file is the original base-project README.

A bare CMake project for the STM32H563 running Eclipse ThreadX with the ST HAL.
The pin defaults match the NUCLEO-H563ZI.

## Build

```sh
cmake -S . -B build                    # Debug by default; add -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This needs `arm-none-eabi-gcc` on PATH, or `-DARM_TOOLCHAIN_PATH=/path/to/bin`.
The build writes `build/stm32h563_threadx.{elf,hex,bin,map}`.

## Flash

- **STM32CubeProgrammer:** `cmake --build build --target flash`. This target only exists when `STM32_Programmer_CLI` is on PATH.
- **OpenOCD:** `openocd -f interface/stlink.cfg -f target/stm32h5x.cfg -c "program build/stm32h563_threadx.elf verify reset exit"`

## Layout

| Path | What |
|---|---|
| `Core/Src/main.c` | HAL init, 250 MHz clock from HSI+PLL1, GPIO, `tx_kernel_enter()` |
| `Core/Src/app_threadx.c` | `tx_application_define()`: byte pool, threads, semaphore (**your code goes here**) |
| `Core/Inc/board.h` | Pin map (change for a custom board) |
| `Core/Inc/tx_user.h` | ThreadX config (1 kHz tick, stack checking, …) |
| `Core/Inc/stm32h5xx_hal_conf.h` | Which HAL modules are enabled |
| `Core/Startup/tx_initialize_low_level.S` | ThreadX low-level init: vector table, SysTick, exception priorities |
| `Core/Src/stm32h5xx_hal_timebase_tim.c` | HAL tick on TIM6 |
| `Core/Src/syscalls.c` | newlib stubs; implement `__io_putchar()` to retarget `printf` |
| `STM32H563xx_FLASH.ld` | Linker script (2 MB flash / 640 KB RAM) |
| `third_party/` | Submodules: ThreadX v6.4.5, CMSIS core 5.9, cmsis-device-h5 v1.7.0, stm32h5xx HAL v1.7.0 |

## Notes

- **Timer ownership.** ThreadX owns SysTick at 1 ms per tick. The HAL tick (`HAL_Delay`, HAL timeouts) runs on TIM6. Don't use TIM6 for anything else.
- **Handlers you must not redefine.** ThreadX provides `PendSV_Handler` and `SysTick_Handler`.
- **Interrupt priorities.** `__NVIC_PRIO_BITS` is 4. PendSV and SVC are set to the lowest priority (15). SysTick is 4. The TIM6 HAL tick is 15.
- **ISRs and ThreadX.** An ISR may call ThreadX services such as `tx_semaphore_put` or `tx_queue_send` with `TX_NO_WAIT`. Don't call `HAL_Delay()` from an ISR.
- **TrustZone.** The build assumes TrustZone is off (TZEN=0, the factory default) via `TX_SINGLE_MODE_NON_SECURE`.
- **Flash size.** For a 1 MB part (STM32H563xG), set FLASH `LENGTH = 1024K` in the linker script.
- **Clock source.** The clock uses the internal HSI, so it works on any board. To use a crystal, switch to HSE in `SystemClock_Config()` and set `HSE_VALUE`.
