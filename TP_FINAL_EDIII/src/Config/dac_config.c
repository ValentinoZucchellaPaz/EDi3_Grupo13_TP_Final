#include "dac_config.h"

/*---------------------------------------------------------
 * Variables privadas
 *--------------------------------------------------------*/

static volatile uint16_t buzzer_amplitud = 0;
static volatile uint8_t buzzer_enable = 0;

/*---------------------------------------------------------
 * Configuración DAC
 *--------------------------------------------------------*/
void DAC_Config(void)
{
    DAC_Init();

    DAC_SetBias(DAC_700uA);

    DAC_UpdateValue(0);

    /*-----------------------------------------------------
     * Timer3
     * Frecuencia inicial: 1 kHz
     *
     * T = 1 ms
     * T/2 = 500 us
     *----------------------------------------------------*/

    TIM_TIMERCFG_T timCfg;

    timCfg.prescaleOpt = TIM_US;
    timCfg.prescaleValue = 1;

    TIM_MATCHCFG_T matchCfg;

    matchCfg.channel = TIM_MATCH_0;
    matchCfg.intEn = ENABLE;
    matchCfg.resetEn = ENABLE;
    matchCfg.stopEn = DISABLE;
    matchCfg.extOpt = TIM_NOTHING;
    matchCfg.matchValue = 500;

    TIM_InitTimer(LPC_TIM3, &timCfg);

    TIM_ConfigMatch(LPC_TIM3, &matchCfg);

    NVIC_EnableIRQ(TIMER3_IRQn);

    TIM_Enable(LPC_TIM3);
}

/*---------------------------------------------------------
 * Relación distancia -> sonido
 *--------------------------------------------------------*/
void DAC_SetDistance(uint16_t distance)
{
    if(distance > 100){
        buzzer_enable = 0;
        buzzer_amplitud = 0;
        return;
    }

    buzzer_enable = 1;

    /*---------------------------------------------
     * Más cerca = mayor amplitud
     *--------------------------------------------*/

    buzzer_amplitud = 1023 - ((distance * 1023) / 100);

    if(buzzer_amplitud > 1023) buzzer_amplitud = 1023;

    /*---------------------------------------------
     * Más cerca = frecuencia más alta
     *
     * 100 cm -> 500 Hz
     * 0 cm   -> 4000 Hz
     *--------------------------------------------*/

    uint32_t frecuencia;

    frecuencia = 500 + ((100 - distance) * 3500UL) / 100UL;

    if(frecuencia < 500) frecuencia = 500;

    if(frecuencia > 4000) frecuencia = 4000;

    uint32_t half_period_us;

    half_period_us = 500000UL / frecuencia;

    TIM_UpdateMatchValue(LPC_TIM3,TIM_MATCH_0,half_period_us);
}

/*---------------------------------------------------------
 * ISR Timer3
 *
 * Genera onda cuadrada:
 *
 * 0
 * amplitud
 * 0
 * amplitud
 *
 * usando el DAC.
 *--------------------------------------------------------*/
void TIMER3_IRQHandler(void)
{
    static uint8_t estado = 0;

    if(TIM_GetIntStatus(LPC_TIM3,TIM_MR0_INT) == SET){
        TIM_ClearIntPending(LPC_TIM3,TIM_MR0_INT);

        if(!buzzer_enable){
            DAC_UpdateValue(0);
            return;
        }

        estado ^= 1;

        if(estado){
            DAC_UpdateValue(buzzer_amplitud);
        }else{
            DAC_UpdateValue(0);
        }
    }
}