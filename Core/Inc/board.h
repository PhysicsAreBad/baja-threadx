/*
 * Board pin map. Defaults match the NUCLEO-H563ZI; change these for a custom board.
 */
#ifndef BOARD_H
#define BOARD_H

#include "stm32h5xx_hal.h"

/* LD1 (green) */
#define LED_GREEN_PORT          GPIOB
#define LED_GREEN_PIN           GPIO_PIN_0
#define LED_GREEN_CLK_ENABLE()  __HAL_RCC_GPIOB_CLK_ENABLE()

/* LD2 (yellow) */
#define LED_YELLOW_PORT         GPIOF
#define LED_YELLOW_PIN          GPIO_PIN_4
#define LED_YELLOW_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()

/* LD3 (red), used by Error_Handler() */
#define LED_RED_PORT            GPIOG
#define LED_RED_PIN             GPIO_PIN_4
#define LED_RED_CLK_ENABLE()    __HAL_RCC_GPIOG_CLK_ENABLE()

/* Analog input: PA6 = ADC1_IN11 (Arduino A1 on the NUCLEO-H563ZI) */
#define ADC_INPUT_PORT          GPIOA
#define ADC_INPUT_PIN           GPIO_PIN_6
#define ADC_INPUT_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define ADC_INPUT_CHANNEL       ADC_CHANNEL_11

#endif /* BOARD_H */
