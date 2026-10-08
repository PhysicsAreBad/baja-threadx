/*
 * ADC1 driver for one analog pin, using the HAL in polling mode.
 *
 * Walkthrough step 1: this file knows nothing about ThreadX. It is plain HAL code
 * that a thread calls. Everything here runs once at start-up, except
 * ADC_Sensor_Read() which a thread calls repeatedly.
 */
#include "adc_sensor.h"
#include "board.h"
#include "main.h"

static ADC_HandleTypeDef hadc1;

void ADC_Sensor_Init(void)
{
    RCC_PeriphCLKInitTypeDef pclk = {0};
    GPIO_InitTypeDef gpio = {0};
    ADC_ChannelConfTypeDef ch = {0};

    /* 1. Kernel clock for the ADC (the HAL needs this even in synchronous mode). */
    pclk.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
    pclk.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_HCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK)
    {
        Error_Handler();
    }
    __HAL_RCC_ADC_CLK_ENABLE();

    /* 2. Put the pin in analog mode (no pull, so it does not load the signal). */
    ADC_INPUT_CLK_ENABLE();
    gpio.Pin  = ADC_INPUT_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ADC_INPUT_PORT, &gpio);

    /* 3. ADC1: 12 bit, software-triggered, one channel, one conversion per Start. */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;   /* 250 MHz / 4 */
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.SamplingMode          = ADC_SAMPLING_MODE_NORMAL;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode      = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 4. Self-calibration (must be done with the ADC disabled, i.e. before first Start). */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }

    /* 5. Which pin/channel to convert, and how long to let the sampling cap charge. */
    ch.Channel      = ADC_INPUT_CHANNEL;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;   /* raise this for high-impedance sources */
    ch.SingleDiff   = ADC_SINGLE_ENDED;
    ch.OffsetNumber = ADC_OFFSET_NONE;
    if (HAL_ADC_ConfigChannel(&hadc1, &ch) != HAL_OK)
    {
        Error_Handler();
    }
}

int ADC_Sensor_Read(uint16_t *raw)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return -1;
    }
    /* One conversion is ~1 us here, so polling with a 10 ms timeout is fine. HAL timeouts
     * use the HAL tick (TIM6), which keeps running under ThreadX. */
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return -1;
    }
    *raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return 0;
}
