#ifndef ULTRASONIC_CONFIG_H
#define ULTRASONIC_CONFIG_H

#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"

void Ultrasonic_Init(void);
uint16_t Ultrasonic_GetDistance(void);

#endif
