#include "ultrasonic_config.h"
#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"

// ---------------------------------------------------------------------------------------
// CONFIGURO PWM DE TIMER PARA PIN TRIG DEL SENSOR
// ---------------------------------------------------------------------------------------

/**
 * @brief Configura pulso para que sensor comience una muestra:
 *
 * Configura el pin P0.18 como GPIO output
 * Configura TIMER2 va a hacer un PWM de periodo 60ms con ese pin y le da start.
 * Conectar a pin TRIG del sensor para que comience una muestra.
 */
static void config_timer1_pwm(void)
{
    // PWM periodo 60ms y high_time 10us (datasheet de HC-SR04)
    // se hace toggle de pin P0.18 en la ISR

    // config pin P1.0 como salida gpio
    GPIO_SetDir(PORT_0, 1 << 18, GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_18, DISABLE);

    // conf timer:
    // match 0 cuenta high_time e interrumpe, match 1 es el periodo (reset e interrumpe)
    TIM_TIMERCFG_T tim;
    tim.prescaleOpt = TIM_US;
    tim.prescaleValue = 1; // TC aumenta cada 1us

    TIM_MATCHCFG_T match0cfg;
    match0cfg.channel = TIM_MATCH_0;
    match0cfg.intEn = ENABLE;
    match0cfg.stopEn = DISABLE;
    match0cfg.resetEn = DISABLE;
    match0cfg.extOpt = TIM_NOTHING;
    match0cfg.matchValue = 10;

    // TIM_MATCHCFG_T match1cfg; // periodo
    // match1cfg.channel = TIM_MATCH_1;
    // match1cfg.intEn = ENABLE;
    // match1cfg.stopEn = DISABLE;
    // match1cfg.resetEn = ENABLE;
    // match1cfg.extOpt = TIM_NOTHING;
    // match1cfg.matchValue = 60000; // 60ms

    // reset de timer
    TIM_InitTimer(LPC_TIM1, &tim);
    TIM_Disable(LPC_TIM1);
    TIM_ResetCounter(LPC_TIM1);
    // config y prendo
    // TIM_ConfigMatch(LPC_TIM1, &match1cfg);
    TIM_ConfigMatch(LPC_TIM1, &match0cfg);
    NVIC_EnableIRQ(TIMER2_IRQn);
}

// pwm isr handler
void TIMER1_IRQHandler()
{
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT) == SET) // high time termino, pongo 0
    {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        GPIO_SetPinState(PORT_0, PIN_18, 0);
    }

    // if (TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT) == SET) // se cumplio periodo, pongo 1
    // {
    //     TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
    //     GPIO_SetPinState(PORT_0, PIN_18, 1);
    // }
}

// ---------------------------------------------------------------------------------------
// CONFIGURO CAPTURE PARA PIN ECHO DEL SENSOR
// ---------------------------------------------------------------------------------------

static volatile uint32_t start_pulse = 0;  // tiempo de rising edge de pwm
static volatile uint32_t end_pulse = 0;    // tiempo de falling edge de pwm
static volatile uint32_t high_time = 0;    // end_pulse - start_pulse
static volatile uint32_t distance_cm = -1; // distancia en cm
static volatile uint32_t pin_status = 0;   // estado de pin

/**
 * @brief Configura el timer1 para que aumente la cuenta cada 1us y configura el CAP2.0 (P0.4) capture e interrumpa en flanco asc y desc
 *
 * La muestra se guarda en `distance_cm`
 */
static void config_timer2_capture(void)
{
    // configuro capture
    TIM_TIMERCFG_T timer_cfg;
    timer_cfg.prescaleOpt = TIM_US;
    timer_cfg.prescaleValue = 1;

    // cap0
    TIM_CAPTURECFG_T cap_cfg;
    cap_cfg.channel = TIM_CAPTURE_0;
    cap_cfg.risingEn = ENABLE;
    cap_cfg.fallingEn = ENABLE;
    cap_cfg.intEn = ENABLE;

    TIM_InitTimer(LPC_TIM2, &timer_cfg);
    // reset de timer
    TIM_Disable(LPC_TIM2);
    TIM_ResetCounter(LPC_TIM2);

    // config y enciendo
    TIM_PinConfig(TIM_CAP2_0_P0_4);
    TIM_ConfigCapture(LPC_TIM2, &cap_cfg);
    NVIC_EnableIRQ(TIMER2_IRQn);
}

/**
 * @brief Cuando viene una interrupcion de timer (capture), veo si fue rising o falling (segun el estado del pin), mido high-time y saco distancia.
 * Finalmente prendo o apago led segund distancia
 */
void TIMER2_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM2, TIM_CR0_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM2, TIM_CR0_INT);
        pin_status = (GPIO_ReadValue(PORT_0) & (1 << 4)) >> 4;

        if (pin_status == 1) // rising edge
        {
            start_pulse = TIM_GetCaptureValue(LPC_TIM2, TIM_CAPTURE_0);
        }
        else // falling edge
        {
            end_pulse = TIM_GetCaptureValue(LPC_TIM2, TIM_CAPTURE_0);

            // manejo ovf
            if (end_pulse < start_pulse)
                high_time = (0xFFFFFFFF - start_pulse) + end_pulse;
            else
                high_time = end_pulse - start_pulse;

            distance_cm = high_time / 58; // datasheet
        }
    }
}

// ---------------------------------------------------------------------------------------
// HAGO FUNCIONES PUBLICAS
// ---------------------------------------------------------------------------------------

/**
 * @brief
 * #### Configurar salida de placa (P1.0) a pin TRIG de sensor
 *
 * P1.0 es GPIO output que hace PWM 60ms periodo y 10us hightime (Timer2 con interrupciones), se siguen las indicaciones del datasheet del sensor)
 * Configura pin
 *
 * #### Configurar pin ECHO de sensor a entrada de placa (P1.18)
 *
 * P1.18 es CAP1.0 (Timer1), dispara interrupcion en ambos flanco, saca distancia con tiempo pasado y guarda en variable static
 * Además si la distancia es menor a 20cm prende el led rojo de la placa (P0.22) sino apaga
 */
void Ultrasonic_Init(void)
{
    config_timer1_pwm();
    config_timer2_capture();
}

/**
 * @brief Comienza timers desde reset y bloquea ejecucion hasta tener un resultado de distancia
 * @return distancia_cm
 */
int Ultrasonic_GetDistance(void)
{
    TIM_ResetCounter(LPC_TIM1);
    TIM_ResetCounter(LPC_TIM2);
    // pongo en 1 P0.18 (trig)
    GPIO_SetPinState(PORT_0, PIN_18, 1);
    TIM_Enable(LPC_TIM1);
    TIM_Enable(LPC_TIM2);
    while (distance_cm == -1)
    {
    };
    TIM_Disable(LPC_TIM1);
    TIM_Disable(LPC_TIM2);
    int aux = distance_cm;
    distance_cm = -1;
    return aux;
}
