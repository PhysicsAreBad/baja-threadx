#include "main.h"
#include "board.h"
#include "tx_api.h"

static void SystemClock_Config(void);
static void GPIO_Init(void);

int main(void)
{
    /* HAL_Init() starts the HAL tick on TIM6 (see stm32h5xx_hal_timebase_tim.c). */
    HAL_Init();
    SystemClock_Config();

    /* Instruction cache: big win when executing from flash. */
    if (HAL_ICACHE_Enable() != HAL_OK)
    {
        Error_Handler();
    }

    GPIO_Init();

    /* Never returns. Calls tx_application_define() (app_threadx.c), then starts the scheduler. */
    tx_kernel_enter();

    while (1)
    {
    }
}

/*
 * SYSCLK = 250 MHz from HSI (64 MHz) via PLL1:
 *   64 MHz / M(16) = 4 MHz  ->  x N(125) = 500 MHz VCO  ->  / P(2) = 250 MHz
 * Uses no external crystal so it runs on any board. To use HSE instead, change
 * the oscillator/PLL source and HSE_VALUE in stm32h5xx_hal_conf.h.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
    {
    }

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSIDiv              = RCC_HSI_DIV1;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLL1_SOURCE_HSI;
    osc.PLL.PLLM            = 16;
    osc.PLL.PLLN            = 125;
    osc.PLL.PLLP            = 2;
    osc.PLL.PLLQ            = 2;
    osc.PLL.PLLR            = 2;
    osc.PLL.PLLRGE          = RCC_PLL1_VCIRANGE_2;  /* 4-8 MHz input  */
    osc.PLL.PLLVCOSEL       = RCC_PLL1_VCORANGE_WIDE;
    osc.PLL.PLLFRACN        = 0;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    clk.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    LED_GREEN_CLK_ENABLE();
    LED_YELLOW_CLK_ENABLE();
    LED_RED_CLK_ENABLE();

    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = LED_GREEN_PIN;
    HAL_GPIO_Init(LED_GREEN_PORT, &gpio);
    gpio.Pin = LED_YELLOW_PIN;
    HAL_GPIO_Init(LED_YELLOW_PORT, &gpio);
    gpio.Pin = LED_RED_PIN;
    HAL_GPIO_Init(LED_RED_PORT, &gpio);
}

void Error_Handler(void)
{
    __disable_irq();
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
    for (;;)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
    Error_Handler();
}
#endif
