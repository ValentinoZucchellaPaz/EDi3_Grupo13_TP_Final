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
#include "ultrasonic_config.h"
#endif

// #include <cr_section_macros.h>

#include <stdio.h>

// parte 1: enviar pulso 10us a TRIG del sensor -> toggle de P1.0 (uso gpio e interrupciones de timer) y ese pin va al match
// parte 2: activar contador y esperar capture, luego mido tiempo
// parte 3: encapsular logica en funcion pulseln como en arduino

// conexiones, el echo devuelve 5v, entonces debo usar resistencias para no quemar el gpio de la placa

int main(void)
{
    Ultrasonic_Init();
    while (1)
    {
    }
    return 0;
}
