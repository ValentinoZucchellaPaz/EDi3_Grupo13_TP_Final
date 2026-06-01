#ifndef TIMER_PWM_H
#define TIMER_PWM_H

#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include <stdint.h>

// ── Configuración física del servo ──────────────────────
#define SERVO_MIN_US    1000     // 1ms  →   0°
#define SERVO_MAX_US    2000     // 2ms  → 180°
#define SERVO_PERIOD_US 20000    // 20ms   → 50Hz

// ── Límites angulares del barrido ───────────────────────
#define SERVO_ANGULO_MIN    0
#define SERVO_ANGULO_MAX  180

// ── Paso del barrido automático ─────────────────────────
#define SERVO_PASO_AUTO     5   // grados por tick en modo auto

// ── Zona muerta del joystick ────────────────────────────
#define JOYSTICK_CENTRO    2048
#define JOYSTICK_ZONA_MUERTA 400  // ±400 cuentas ADC = quieto

// ── Modos del servo ─────────────────────────────────────
typedef enum {
    SERVO_MODO_AUTO,
    SERVO_MODO_MANUAL
} ServoModo_t;

// ── Variables públicas ───────────────────────────────────
extern volatile uint8_t    servo_angulo;   // 0 a 180
extern volatile ServoModo_t servo_modo;

// ── API pública ──────────────────────────────────────────
void configTimerPWM(void);
void servoSetAngulo(uint8_t angulo);
void servoSetModo(ServoModo_t modo);

// Llamar desde SysTick cada 50ms
void servoTick(uint16_t joystick_raw);

#endif