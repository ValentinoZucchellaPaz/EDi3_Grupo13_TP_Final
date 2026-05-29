// 1. Siempre incluís tu propio header primero
#include "dac_config.h"

// 2. Variables globales del módulo
//    Si pusiste extern en el .h, acá va la definición real
volatile uint16_t dac_valor_actual = 0;

// 3. Implementación completa de cada función declarada en el .h
void configDAC(void){
    DAC_Init();
    DAC_SetBias(DAC_350uA);   // menor consumo
}

void dacSetTono(uint16_t distancia_cm){
    // objeto cerca → tono agudo (DAC alto)
    // objeto lejos → tono grave (DAC bajo)
    uint16_t valor = DAC_BITS - (distancia_cm * DAC_BITS / DISTANCIA_MAX_CM);
    DAC_UpdateValue(valor);
    dac_valor_actual = valor;
}