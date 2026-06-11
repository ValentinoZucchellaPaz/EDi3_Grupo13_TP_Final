#ifndef SYSTICK_CFG_H
#define SYSTICK_CFG_H
#include "lpc17xx_systick.h"
#include "lpc17xx_pinsel.h"
#include <stdint.h>




extern volatile uint8_t flag_50ms;

void configSystick(void);
#endif
