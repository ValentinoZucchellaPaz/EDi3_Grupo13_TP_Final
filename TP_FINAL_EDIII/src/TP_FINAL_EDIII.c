#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "Config/ultrasonic_config.h"
#include "Config/dac_config.h"
#include "Config/uart_config.h"
#include "Config/servo_pwm.h"
#endif

#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE 25

typedef struct
{
    uint16_t angulo;
    uint16_t distancia;
} radar_sample_t;

radar_sample_t radar_buffer[BUFFER_SIZE];

int main(void)
{
    GPIO_SetDir(PORT_0, 1 << 22, GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_22, 1);

    Ultrasonic_Init();
    DAC_Config();
    UART0_Config();

    uint16_t idx = 0;
    uint16_t angulo_servo = 0;
    uint16_t distance = 0;

    UART0_SendString("while\r\n");

    while (1)
    {
       UART0_SendString("ENTRE LOOP\r\n");
       distance = Ultrasonic_GetDistance();
       UART0_SendString("SALI DISTANCE\r\n");DAC_SetDistance(distance);
       DAC_SetDistance(distance);

       angulo_servo = servoGetAngulo();
        // Guardar muestra en buffer
        radar_buffer[idx].angulo = angulo_servo;
        radar_buffer[idx].distancia = distance;

        idx++;

        if (angulo_servo > 180){
        	angulo_servo = 0;
        }


        if (idx == BUFFER_SIZE)
        {
            UART0_SendString("MANDO BUFFER\r\n");

            UART0_SendBuffer(
                (uint8_t*)radar_buffer,
				sizeof(radar_buffer)
            );

            idx = 0;

            // delay para no saturar
            for (volatile int i = 0; i < 5000000; i++);
        }
    }

    return 0;
}


/*
buffer nuevo
↓
configuro DMA completo
↓
start
↓
IRQ termina
↓
dma_busy = 0
*/