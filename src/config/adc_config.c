#include "adc_config.h"

void configADC(void){
    ADC_Init(JOY_ADC_RATE); //100khz
    ADC_PinConfig(JOY_ADC_CHANNEL); //canal 1 - AD0.1 → P0.24
    ADC_PowerUp();
    ADC_ChannelEnable(JOY_ADC_CHANNEL);
}

uint16_t adcLeerJoystick(void){
    //Disparo por software
    ADC_StartCmd(ADC_START_NOW);

    // Esperar a que termine la conversión
    // En 100KHz tarda máximo 65 ciclos = 6.5µs → aceptable
    while(ADC_ChannelGetStatus(JOY_ADC_CHANNEL, ADC_DATA_DONE) != SET);

    // Leer resultado de 12 bits (0..4095)
    return (uint16_t)ADC_ChannelGetData(JOY_ADC_CHANNEL);
}