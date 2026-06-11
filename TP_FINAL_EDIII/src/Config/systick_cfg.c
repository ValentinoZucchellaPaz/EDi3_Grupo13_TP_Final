#include "systick_cfg.h"
#include <stdint.h>
volatile uint8_t flag_50ms = 0;

void configSystick(void){
    SYSTICK_InternalInit(10);
    SYSTICK_Cmd(ENABLE);
    SYSTICK_IntCmd(ENABLE);
}

//Handler del Systick, pasado 50ms levanta bandera
void SysTick_Handler(void){
    static uint8_t cnt = 0;
    cnt++;
    if(cnt >= 5){      // SysTick cada 10ms → 5 × 10ms = 50ms
        cnt = 0;
        flag_50ms = 1;  //levanto bandera para el main
    }
}
