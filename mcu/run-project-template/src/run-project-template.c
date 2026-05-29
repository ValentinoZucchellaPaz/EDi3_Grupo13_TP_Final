/*
 * Copyright 2022 NXP
 * NXP confidential.
 * This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms.  By expressly accepting
 * such terms or by downloading, installing, activating and/or otherwise using
 * the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to
 * be bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software.
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

volatile uint32_t duty = 1000; // 1ms -> 0°
volatile uint8_t subiendo = 1;

void confTimerPWM(uint32_t duty);
void confPin();

int main(void)
{

    confPin();
    confTimerPWM(duty);

    while (1)
    {
    }
    return 0;
}

void confPin()
{
    PINSEL_CFG_T pinCfg = {
        .func = 0,
        .mode = PINSEL_TRISTATE,
        .pin = PIN_0,
        .port = PORT_0};
    PINSEL_ConfigPin(&pinCfg);
    GPIO_SetDir(PORT_0, PIN_0, GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_0, SET);
}

void confTimerPWM(uint32_t duty)
{
    TIM_TIMERCFG_T timerCfg = {TIM_US, 1};
    TIM_InitTimer(LPC_TIM1, &timerCfg);

    TIM_MATCHCFG_T match1Cfg = {0}; // match 1 es el periodo
    match1Cfg.channel = TIM_MATCH_1;
    match1Cfg.intEn = ENABLE;
    match1Cfg.stopEn = DISABLE;
    match1Cfg.resetEn = ENABLE;
    match1Cfg.matchValue = 20000;

    TIM_MATCHCFG_T match0Cfg = {0}; // match 0 es el ciclo de trabajo
    match0Cfg.channel = TIM_MATCH_0;
    match0Cfg.intEn = ENABLE;
    match0Cfg.stopEn = DISABLE;
    match0Cfg.resetEn = DISABLE;
    match0Cfg.matchValue = duty;

    TIM_ConfigMatch(LPC_TIM1, &match0Cfg);
    TIM_ConfigMatch(LPC_TIM1, &match1Cfg);
    NVIC_EnableIRQ(TIMER1_IRQn);
    TIM_Enable(LPC_TIM1);
}

void TIMER1_IRQHandler()
{
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        // bajo pin
        GPIO_SetPinState(PORT_0, PIN_0, 0);
    }
    else if (TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
        // subo pin
        GPIO_SetPinState(PORT_0, PIN_0, 1);

        // =========================
        // ACTUALIZO DUTY
        // =========================

        if (subiendo)
        {
            duty += 10;

            if (duty >= 2000)
            {
                duty = 2000;
                subiendo = 0;
            }
        }
        else
        {
            duty -= 10;

            if (duty <= 1000)
            {
                duty = 1000;
                subiendo = 1;
            }
        }

        // actualizo MR0
        TIM_UpdateMatchValue(LPC_TIM1, TIM_MATCH_0, duty);
    }
}
