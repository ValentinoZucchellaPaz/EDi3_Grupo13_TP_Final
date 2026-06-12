#ifndef DAC_CONFIG_H_
#define DAC_CONFIG_H_

#include "lpc17xx_dac.h"
#include <stdint.h>

void DAC_Config(void);
void DAC_SetDistance(uint16_t distance);

#endif