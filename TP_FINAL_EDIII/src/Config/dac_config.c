#include "dac_config.h"

void DAC_Config(void)
{
    DAC_Init();
    DAC_SetBias(DAC_700uA);
    DAC_UpdateValue(0);
}

void DAC_SetDistance(uint16_t distance)
{
    uint16_t dac_value;

    if(distance <= 5)
        dac_value = 1023;
    else if(distance <= 10)
        dac_value = 900;
    else if(distance <= 20)
        dac_value = 700;
    else if(distance <= 30)
        dac_value = 500;
    else if(distance <= 40)
        dac_value = 350;
    else if(distance <= 50)
        dac_value = 250;
    else if(distance <= 70)
        dac_value = 100;
    else
        dac_value = 0;

    DAC_UpdateValue(dac_value);
}