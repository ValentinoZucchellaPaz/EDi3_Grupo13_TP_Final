#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include <stdint.h>

// ── Pin del joystick ────────────────────────────────────
// Eje X → AD0.1 → P0.24 → PINSEL1 bits[17:16] = 01
#define JOY_ADC_CHANNEL     ADC_CHANNEL_1
#define JOY_ADC_RATE        100000        //100khz

// ── Zona muerta del joystick (centro ± esta cantidad) ───
// ADC 12 bits → rango 0 a 4095, centro en reposo ≈ 2048
#define JOY_CENTRO          2048
#define JOY_ZONA_MUERTA     300             // ±300 cuentas = quieto

// ── Inicialización ──────────────────────────────────────
void configADC(void);

// ── Lectura del joystick ────────────────────────────────
// Retorna valor crudo 0..4095
// Centro (reposo) ≈ 2048
// Izquierda       → valores bajos  (hacia 0)
// Derecha         → valores altos  (hacia 4095)
uint16_t adcLeerJoystick(void);

#endif