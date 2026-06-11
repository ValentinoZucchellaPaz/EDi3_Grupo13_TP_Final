#include "timer_pwm.h"
#include "LPC17xx.h"
#include <stdint.h>

volatile uint8_t     servo_angulo = 0;
volatile ServoModo_t servo_modo   = SERVO_MODO_AUTO;

static volatile uint32_t pulso_us = 500;
static int8_t  direccion          = 1;
static uint8_t cnt_periodos       = 0;

static uint32_t anguloAPulso(uint8_t angulo)
{
    return SERVO_MIN_US
         + ((uint32_t)angulo * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
}

void configTimerPWM(void)
{
    /* ── GPIO ────────────────────────────────────────────── */
    PINSEL_CFG_T pinCfg = {
        .port      = PORT_0,
        .pin       = PIN_0,
        .func      = PINSEL_FUNC_00,
        .mode      = PINSEL_PULLUP,
        .openDrain = DISABLE
    };
    GPIO_SetDir(PORT_0, (1 << 0), GPIO_OUTPUT);
    GPIO_ClearPins(PORT_0, (1 << 0));
    PINSEL_ConfigPin(&pinCfg);

    /* ── Timer0: período de 20 ms ────────────────────────── */
    LPC_SC->PCONP    |=  (1 << 1);        /* encender Timer0  */
    LPC_SC->PCLKSEL0 &= ~(3 << 2);        /* PCLK = CCLK/4 = 25 MHz */

    LPC_TIM0->TCR = (1 << 1);             /* reset             */
    LPC_TIM0->PR  = 24;                   /* prescaler: 25MHz/(24+1) = 1 MHz → 1 tick = 1 µs */
    LPC_TIM0->MR0 = SERVO_PERIOD_US - 1; /* 19999 µs → 20 ms  */
    LPC_TIM0->MCR = (1 << 0)             /* IRQ on MR0        */
                  | (1 << 1);             /* reset TC on MR0   */
    LPC_TIM0->TCR = (1 << 0);             /* start             */

    /* ── Timer1: duración del pulso (one-shot) ───────────── */
    LPC_SC->PCONP    |=  (1 << 2);        /* encender Timer1   */
    LPC_SC->PCLKSEL0 &= ~(3 << 4);        /* PCLK = CCLK/4 = 25 MHz */

    LPC_TIM1->TCR = (1 << 1);             /* reset             */
    LPC_TIM1->PR  = 24;                   /* mismo prescaler   */
    LPC_TIM1->MR0 = pulso_us;
    LPC_TIM1->MCR = (1 << 0)             /* IRQ on MR0        */
                  | (1 << 1)             /* reset TC on MR0   */
                  | (1 << 2);             /* stop TC on MR0    */
    /* Timer1 arranca detenido — Timer0 lo dispara */

    /* ── IRQ ─────────────────────────────────────────────── */
    NVIC_SetPriority(TIMER0_IRQn, 1);
    NVIC_SetPriority(TIMER1_IRQn, 0);     /* Timer1 prioridad más alta */
    NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);
}

void servoSetAngulo(uint8_t angulo)
{
    if (angulo > SERVO_ANGULO_MAX) angulo = SERVO_ANGULO_MAX;
    if (angulo < SERVO_ANGULO_MIN) angulo = SERVO_ANGULO_MIN;
    servo_angulo = angulo;
    pulso_us     = anguloAPulso(angulo);
}

void servoSetModo(ServoModo_t modo)
{
    servo_modo = modo;
    if (modo == SERVO_MODO_AUTO) direccion = 1;
}

void servoTick(uint16_t joystick_raw)
{
    if (servo_modo == SERVO_MODO_MANUAL)
    {
        int16_t desviacion = (int16_t)joystick_raw - (int16_t)JOYSTICK_CENTRO;

        if (desviacion > (int16_t)JOYSTICK_ZONA_MUERTA) {
            int16_t paso  = desviacion / 400 + 1;
            int16_t nuevo = (int16_t)servo_angulo + paso;
            if (nuevo > SERVO_ANGULO_MAX) nuevo = SERVO_ANGULO_MAX;
            servoSetAngulo((uint8_t)nuevo);
        } else if (desviacion < -(int16_t)JOYSTICK_ZONA_MUERTA) {
            int16_t paso  = (-desviacion) / 400 + 1;
            int16_t nuevo = (int16_t)servo_angulo - paso;
            if (nuevo < SERVO_ANGULO_MIN) nuevo = SERVO_ANGULO_MIN;
            servoSetAngulo((uint8_t)nuevo);
        }
    }
}

/* ── Timer0 ISR: inicio de período ───────────────────────
   Sube el pin, carga el pulso en Timer1 y lo arranca.
   Timer1 se encarga de bajarlo — sin ninguna colisión.     */
void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = (1 << 0);              /* limpiar flag      */

    /* Avanzar barrido automático */
    if (servo_modo == SERVO_MODO_AUTO)
    {
        cnt_periodos++;
        if (cnt_periodos >= SERVO_PERIODOS_POR_PASO)
        {
            cnt_periodos = 0;

            int16_t nuevo = (int16_t)servo_angulo
                          + (int16_t)(direccion * SERVO_PASO_AUTO);

            if (nuevo >= (int16_t)SERVO_ANGULO_MAX) {
                nuevo     = SERVO_ANGULO_MAX;
                direccion = -1;
            } else if (nuevo <= (int16_t)SERVO_ANGULO_MIN) {
                nuevo     = SERVO_ANGULO_MIN;
                direccion =  1;
            }

            servo_angulo = (uint8_t)nuevo;
            pulso_us     = anguloAPulso((uint8_t)nuevo);
        }
    }

    /* Subir pin */
    LPC_GPIO0->FIOCLR = (1 << 0);

    /* Cargar duración del pulso en Timer1 y arrancarlo */
    LPC_TIM1->TCR = (1 << 1);             /* reset Timer1      */
    LPC_TIM1->MR0 = pulso_us;
    LPC_TIM1->TCR = (1 << 0);             /* start Timer1      */
}

/* ── Timer1 ISR: fin del pulso ───────────────────────────
   Baja el pin. Timer1 se detuvo solo (stop on match).      */
void TIMER1_IRQHandler(void)
{
    LPC_TIM1->IR = (1 << 0);              /* limpiar flag      */
    LPC_GPIO0->FIOSET = (1 << 0);         /* P0.0 = LOW        */
}
