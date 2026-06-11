#ifndef DAC_CONFIG_H
#define DAC_CONFIG_H

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpio.h"

// Inicialización del DAC
void DAC_Config(void);

// Actualiza la distancia medida (en cm o unidades que uses)
void DAC_SetDistance(uint16_t distance);

#endif
