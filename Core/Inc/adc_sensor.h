#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include <stdint.h>

#define ADC_FULL_SCALE      4095U   /* 12-bit */
#define ADC_VREF_MV         3300U   /* VREF+ on the Nucleo is tied to 3.3 V */

/* Configures PA6 as analog, enables ADC1, runs the calibration. Call before the scheduler. */
void ADC_Sensor_Init(void);

/* Blocking single conversion. Returns 0 on success and writes the raw 12-bit code. */
int ADC_Sensor_Read(uint16_t *raw);

static inline uint32_t ADC_RawToMilliVolts(uint32_t raw)
{
    return (raw * ADC_VREF_MV) / ADC_FULL_SCALE;
}

#endif /* ADC_SENSOR_H */
