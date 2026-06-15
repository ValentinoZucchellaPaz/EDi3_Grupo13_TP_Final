#ifndef SERVO_PWM_H
#define SERVO_PWM_H

#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include <stdint.h>


// ── Configuración física del servo ──────────────────────
#define SERVO_MIN_US    500      // 0.5ms  →   0°
#define SERVO_MAX_US    2500     // 2.5ms  → 180°
#define SERVO_PERIOD_US 20000    // 20ms   → 50Hz

// ── Límites angulares del barrido ───────────────────────
#define SERVO_ANGULO_MIN    0
#define SERVO_ANGULO_MAX  180

// ── Paso del barrido automático ─────────────────────────
#define SERVO_PASO_AUTO     2   // grados por tick en modo auto

// Cada cuántos períodos de 20 ms se avanza un paso
// 3 × 20 ms = 60 ms por paso
#define SERVO_PERIODOS_POR_PASO  3

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
uint8_t servoGetAngulo();
void servoSetAnguloAutomatico();

// Llamar desde SysTick cada 50ms
void servoTick(uint16_t joystick_raw);// LLAMAR SOLO EN MODO MANUAL

#endif
