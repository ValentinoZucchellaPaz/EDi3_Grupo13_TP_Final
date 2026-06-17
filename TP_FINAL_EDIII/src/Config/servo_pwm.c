#include "servo_pwm.h"

volatile uint8_t servo_angulo = 0;
volatile ServoModo_t servo_modo = SERVO_MODO_MANUAL;

static volatile uint32_t pulso_us = 500;
static int8_t direccion = 1;
// static uint8_t cnt_periodos = 0;

/** @brief Funcion auxiliar para obtener `pulso_us` a partir del `angulo` deseado */
static uint32_t anguloAPulso(uint8_t angulo)
{
    return SERVO_MIN_US + ((uint32_t)angulo * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
}

/** @brief PWM con periodo 20ms y high_time `pulso_us` (va desde 0.5ms a 2.5ms, segun datasheet de servo SG90) */
void Servo_ConfigTimerPWM(void)
{
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

/** @brief Cambia el high_time (`pulso_us`) del PWM del servo para llevarlo al `angulo` deseado (de 0° a 180°) */
void Servo_SetAngulo(uint8_t angulo)
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

/** @return `servo_angulo`*/
uint8_t Servo_GetAngulo()
{
    return servo_angulo;
}

/** @brief Cambia `servo_modo` y enciende o apaga el led verde indicador de este (P3.25) */
void Servo_SetModo(ServoModo_t modo)
{
    servo_modo = modo;
    if (modo == SERVO_MODO_AUTO)
    {
        direccion = 1;
        LPC_GPIO3->FIOSET = (1 << 25);
    }
    if (modo == SERVO_MODO_MANUAL)
    {
        LPC_GPIO3->FIOCLR = (1 << 25); // enciendo led en modo manual
    }
}

/** @return `servo_modo`*/
ServoModo_t Servo_GetModo(void)
{
    return servo_modo;
}

/** @brief Cambia `servo_angulo` en modo manual con el valor de adc pasado*/
void Servo_Tick(uint16_t adc_value)
{
    if (servo_modo == SERVO_MODO_MANUAL)
    {
        uint8_t angulo = (adc_value * 180) / 4095;
        Servo_SetAngulo(angulo);
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
        // cnt_periodos++;
    }
}

/** @brief Cambia el angulo del servo con incremento fijo teniendo en cuenta direccion */
void Servo_SetAnguloAutomatico()
{

    // if (cnt_periodos >= SERVO_PERIODOS_POR_PASO) // cambiar
    // {
    //     cnt_periodos = 0;

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
    // }
}
