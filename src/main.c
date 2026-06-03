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
#include "Config/timer_pwm.h"
#include "Config/systick_cfg.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_pinsel.h"

#include <stdint.h>


#include <cr_section_macros.h>

#include <stdio.h>


int main(void)
{
    configTimerPWM();
    //configSystick();
    //servoSetAngulo(90);
    while(1)
    {
    	//if (flag_50ms){
    	//	flag_50ms = 0;
    	//    servoTick(0);
    	//}
    }
}
#endif
