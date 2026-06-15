#include "servo_pwm.h"
#include "LPC17xx.h"
#include <stdint.h>

volatile uint8_t servo_angulo = 0;
volatile ServoModo_t servo_modo = SERVO_MODO_AUTO;

static volatile uint32_t pulso_us = 500;
static int8_t direccion = 1;
static uint8_t cnt_periodos = 0;

static uint32_t anguloAPulso(uint8_t angulo)
{
    return SERVO_MIN_US + ((uint32_t)angulo * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
}

void configTimerPWM(void)
{
    // PWM periodo 20ms y high_time pulso_us (va desde 0.5ms a 2.5ms, datasheet servo SG90)

    // config pin P0.0 como salida gpio
    GPIO_SetDir(PORT_0, (1 << 0), GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_0, DISABLE);

    // conf timer:
    // match 1 cuenta high_time e interrumpe, match 0 es el periodo (reset e interrumpe)
    TIM_TIMERCFG_T tim;
    tim.prescaleOpt = TIM_US;
    tim.prescaleValue = 1; // TC aumenta cada 1us

    TIM_MATCHCFG_T match0cfg; // periodo
    match0cfg.channel = TIM_MATCH_0;
    match0cfg.intEn = ENABLE;
    match0cfg.stopEn = DISABLE;
    match0cfg.resetEn = ENABLE;
    match0cfg.extOpt = TIM_NOTHING;
    match0cfg.matchValue = SERVO_PERIOD_US;

    TIM_MATCHCFG_T match1cfg; // duty
    match1cfg.channel = TIM_MATCH_1;
    match1cfg.intEn = ENABLE;
    match1cfg.stopEn = DISABLE;
    match1cfg.resetEn = DISABLE;
    match1cfg.extOpt = TIM_NOTHING;
    match1cfg.matchValue = pulso_us;

    // reset de timer
    TIM_InitTimer(LPC_TIM0, &tim);
    TIM_Disable(LPC_TIM0);
    TIM_ResetCounter(LPC_TIM0);
    // config y prendo
    TIM_ConfigMatch(LPC_TIM0, &match1cfg);
    TIM_ConfigMatch(LPC_TIM0, &match0cfg);
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Enable(LPC_TIM0);
}

void servoSetAngulo(uint8_t angulo)
{
    if (angulo > SERVO_ANGULO_MAX)
        angulo = SERVO_ANGULO_MAX;
    if (angulo < SERVO_ANGULO_MIN)
        angulo = SERVO_ANGULO_MIN;

    // si modo automatico -> logica
    // si modo manual -> logica

    servo_angulo = angulo;
    pulso_us = anguloAPulso(angulo);
    LPC_TIM0->MR1 = pulso_us;
}

uint8_t servoGetAngulo(){
	return servo_angulo;
}

void servoSetModo(ServoModo_t modo)
{
    servo_modo = modo;
    if (modo == SERVO_MODO_AUTO)
        direccion = 1;
}

void servoTick(uint16_t joystick_raw)
{
    if (servo_modo == SERVO_MODO_MANUAL)
    {
        int16_t desviacion = (int16_t)joystick_raw - (int16_t)JOYSTICK_CENTRO;

        if (desviacion > (int16_t)JOYSTICK_ZONA_MUERTA)
        {
            int16_t paso = desviacion / 400 + 1;
            int16_t nuevo = (int16_t)servo_angulo + paso;
            if (nuevo > SERVO_ANGULO_MAX)
                nuevo = SERVO_ANGULO_MAX;
            servoSetAngulo((uint8_t)nuevo);
        }
        else if (desviacion < -(int16_t)JOYSTICK_ZONA_MUERTA)
        {
            int16_t paso = (-desviacion) / 400 + 1;
            int16_t nuevo = (int16_t)servo_angulo - paso;
            if (nuevo < SERVO_ANGULO_MIN)
                nuevo = SERVO_ANGULO_MIN;
            servoSetAngulo((uint8_t)nuevo);
        }
    }
}

/* ── Timer0 ISR: inicio de período ───────────────────────
   Sube el pin, carga el pulso en Timer1 y lo arranca.
   Timer1 se encarga de bajarlo — sin ninguna colisión.     */
void TIMER0_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT) == SET) // high time termino, pongo 0
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
        GPIO_SetPinState(PORT_0, PIN_0, 0);
    }

    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) // se cumplio periodo, pongo 1
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        GPIO_SetPinState(PORT_0, PIN_0, 1);
    }
}

void servoSetAnguloAutomatico()
{
    cnt_periodos++;
    if (cnt_periodos >= SERVO_PERIODOS_POR_PASO) // cambiar
    {
        cnt_periodos = 0;

        int16_t nuevo = (int16_t)servo_angulo + (int16_t)(direccion * SERVO_PASO_AUTO);

        if (nuevo >= (int16_t)SERVO_ANGULO_MAX)
        {
            nuevo = SERVO_ANGULO_MAX;
            direccion = -1;
        }
        else if (nuevo <= (int16_t)SERVO_ANGULO_MIN)
        {
            nuevo = SERVO_ANGULO_MIN;
            direccion = 1;
        }

        servo_angulo = (uint8_t)nuevo;
        pulso_us = anguloAPulso((uint8_t)nuevo);
        LPC_TIM0->MR1 = pulso_us;
    }
}
