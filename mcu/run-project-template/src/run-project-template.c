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
