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
    return SERVO_MIN_US + ((uint32_t)angulo * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
}

void configTimerPWM(void)
{
    /* GPIO */
    PINSEL_CFG_T pinCfg = {
        .port      = PORT_0,
        .pin       = PIN_0,
        .func      = PINSEL_FUNC_00,
        .mode      = PINSEL_PULLUP,
        .openDrain = DISABLE
    };

    GPIO_SetDir(PORT_0, (1 << 0), GPIO_OUTPUT);
    GPIO_SetPins(PORT_0, (1 << 0));
    PINSEL_ConfigPin(&pinCfg);

    /* Timer0 -> 1 tick = 1 us */
    TIM_TIMERCFG_T timCfg = {
        .prescaleOpt   = TIM_US,
        .prescaleValue = 1
    };

    /*
        MR0:
        Período completo de 20 ms.
    */
    TIM_MATCHCFG_T mr0 = {
        .channel    = TIM_MATCH_0,
        .matchValue = SERVO_PERIOD_US,
        .resetEn    = ENABLE,
        .stopEn     = DISABLE,
        .intEn      = ENABLE,
        .extOpt     = TIM_NOTHING
    };

    /*
        MR1:
        Fin del pulso PWM.
    */
    TIM_MATCHCFG_T mr1 = {
        .channel    = TIM_MATCH_1,
        .matchValue = pulso_us,
        .resetEn    = DISABLE,
        .stopEn     = DISABLE,
        .intEn      = ENABLE,
        .extOpt     = TIM_NOTHING
    };

    TIM_InitTimer(LPC_TIM0, &timCfg);

    TIM_ConfigMatch(LPC_TIM0, &mr0);
    TIM_ConfigMatch(LPC_TIM0, &mr1);

    TIM_Enable(LPC_TIM0);

    NVIC_SetPriority(TIMER0_IRQn, 0);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void servoSetAngulo(uint8_t angulo)
{
    if (angulo > SERVO_ANGULO_MAX) angulo = SERVO_ANGULO_MAX;
    if (angulo < SERVO_ANGULO_MIN) angulo = SERVO_ANGULO_MIN;

    servo_angulo = angulo;
    pulso_us     = anguloAPulso(angulo);

    TIM_UpdateMatchValue(
        LPC_TIM0,
        TIM_MATCH_1,
        pulso_us
    );
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
        int16_t desviacion =
            (int16_t)joystick_raw -
            (int16_t)JOYSTICK_CENTRO;

        if (desviacion > (int16_t)JOYSTICK_ZONA_MUERTA)
        {
            int16_t paso =
                desviacion / 400 + 1;

            int16_t nuevo =
                (int16_t)servo_angulo + paso;

            if (nuevo > SERVO_ANGULO_MAX)
                nuevo = SERVO_ANGULO_MAX;

            servoSetAngulo((uint8_t)nuevo);
        }
        else if (desviacion < -(int16_t)JOYSTICK_ZONA_MUERTA)
        {
            int16_t paso =
                (-desviacion) / 400 + 1;

            int16_t nuevo =
                (int16_t)servo_angulo - paso;

            if (nuevo < SERVO_ANGULO_MIN)
                nuevo = SERVO_ANGULO_MIN;

            servoSetAngulo((uint8_t)nuevo);
        }
    }
}

/*
    Timer0:

    MR0 -> inicio del período
    MR1 -> fin del pulso
*/
void TIMER0_IRQHandler(void)
{
    /*
        MR0 -> comienzo del período PWM
    */
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

        /* Barrido automático */
        if (servo_modo == SERVO_MODO_AUTO)
        {
            cnt_periodos++;

            if (cnt_periodos >= SERVO_PERIODOS_POR_PASO)
            {
                cnt_periodos = 0;

                int16_t nuevo =
                    (int16_t)servo_angulo +
                    (int16_t)(direccion * SERVO_PASO_AUTO);

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
                pulso_us     = anguloAPulso((uint8_t)nuevo);

                TIM_UpdateMatchValue(
                    LPC_TIM0,
                    TIM_MATCH_1,
                    pulso_us
                );
            }
        }

        /*
            Inicio del pulso.
            Igual que tu código original.
        */
        GPIO_ClearPins(PORT_0, (1 << 0));
    }

    /*
        MR1 -> fin del pulso
    */
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);

        GPIO_SetPins(PORT_0, (1 << 0));
    }
}