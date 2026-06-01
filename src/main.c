#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "config/dac_config.h"
#include "config/systick_cfg.h"
#include "config/timer_pwm.h"
#include "config/adc_config.h"
#include "config/uart_config.h"
#include "config/dma_config.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>



int main(void)
{
    GPDMA_Init();

    configSystick();
    configTimerPWM();
    configADC();
    configUART();
    
    while (1){
        // Procesar comandos de la PC
        if(cmd_recibido){
            cmd_recibido = 0;
            switch(ultimo_cmd){
                case CMD_AUTO:   
                    servoSetModo(SERVO_MODO_AUTO);   
                    break;
                case CMD_MANUAL: 
                    servoSetModo(SERVO_MODO_MANUAL); 
                    break;
                default: 
                    break;
            }
        }

        // Ticks del sevo cada 50ms
        if(flag_50ms){
            flag_50ms = 0;
            uint16_t joy = adcLeerJoystick();  // lee ADC joystick
            servoTick(joy);                    // actualiza servo
        }
    }
    
    return 0;
}

