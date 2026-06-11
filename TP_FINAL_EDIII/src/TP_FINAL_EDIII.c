#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "Config/ultrasonic_config.h"
#include "Config/dac_config.h"
#endif

// #include <cr_section_macros.h>

#include <stdio.h>

// parte 1: enviar pulso 10us a TRIG del sensor -> toggle de P1.0 (uso gpio e interrupciones de timer) y ese pin va al match
// parte 2: activar contador y esperar capture, luego mido tiempo
// parte 3: encapsular logica en funcion pulseln como en arduino

// conexiones, el echo devuelve 5v, entonces debo usar resistencias para no quemar el gpio de la placa

int main(void)
{
    // config pin P0.22 como salida gpio: cuando la distancia pase un umbral prende el led, cuando no apaga
    GPIO_SetDir(PORT_0, 1 << 22, GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_22, 1);
    
    Ultrasonic_Init();
    DAC_Config();
    
    int distance = 0;
    while (1)
    {
        // actualizar el duty del pwm_servo -> lo clava en un angulo
        distance = Ultrasonic_GetDistance();
        DAC_SetDistance(distance);
        // mando a uart o lo que vayamos a hacer
        // actualizo duty de angulo
    }
    return 0;
}
