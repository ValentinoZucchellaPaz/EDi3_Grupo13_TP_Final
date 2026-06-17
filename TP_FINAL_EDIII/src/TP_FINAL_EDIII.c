#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "Config/ultrasonic_config.h"
#include "Config/dac_config.h"
#include "Config/uart_config.h"
#include "Config/servo_pwm.h"
#include "Config/adc_config.h"
#endif

#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE 25

typedef struct
{
    uint16_t angulo;
    uint16_t distancia;
} radar_sample_t;

radar_sample_t radar_bufferA[BUFFER_SIZE];
radar_sample_t radar_bufferB[BUFFER_SIZE];

volatile uint8_t flag_buffer = 1; //comienza con buffer A

int main(void)
{
    GPIO_SetDir(PORT_0, 1 << 22, GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_22, 1);

    GPIO_SetDir(PORT_0, 1 << 1, GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_1, 0);

    Ultrasonic_Init();
    DAC_Config();
    ADC_Config();
    UART0_Config();
    configTimerPWM(); //servo



    uint16_t idx = 0;
    uint16_t angulo_servo = 0;
    uint16_t distance = 0;

    while (1)
    {
       //UART0_SendString("ENTRE LOOP\r\n");
       distance = Ultrasonic_GetDistance();
       angulo_servo = servoGetAngulo();
       //UART0_SendString("SALI DISTANCE\r\n");
       
       DAC_SetDistance(distance);	// CAMBIAR ESTO (NO LO DEBERIA HACER EN DAC_CONFIG, POR EL DAC DEBO SACAR VALOR REAL A OSCILOSCOPIO )

       if (servoGetModo() == SERVO_MODO_AUTO)
       {
           servoSetAnguloAutomatico();
       }
       else
       {
           uint16_t adc = ADC_Read();
           servoTick(adc);

           // DEBUG: ver qué está leyendo el ADC
           char buf[40];
           sprintf(buf, "ADC: %u  ANGULO: %u\r\n", adc, servoGetAngulo());
           UART0_SendString(buf);
       }

        // Guardar muestra en buffer
       if(flag_buffer){
    	   radar_bufferA[idx].angulo = angulo_servo;
    	   radar_bufferA[idx].distancia = distance;
       }else{
    	   radar_bufferB[idx].angulo = angulo_servo;
    	   radar_bufferB[idx].distancia = distance;
       }

        idx++;


        if (idx == BUFFER_SIZE)
        {
        	if(flag_buffer){
        		flag_buffer = 0;
        		//UART0_SendString("MANDO BUFFER A\r\n");
        		UART0_SendBuffer((uint8_t*)radar_bufferA,sizeof(radar_bufferA));

        	}else{
        		flag_buffer = 1;
        		//UART0_SendString("MANDO BUFFER B\r\n");
        		UART0_SendBuffer((uint8_t*)radar_bufferB,sizeof(radar_bufferB));
        	}


            idx = 0;

            // delay para no saturar
            //for (volatile int i = 0; i < 5000000; i++);
        }
    }

    return 0;
}
