#include "dac_config.h"
#include "lpc17xx_gpio.h"

void DAC_Config(void)
{
    DAC_Init();
    DAC_SetBias(DAC_700uA);
    DAC_UpdateValue(0);
}

void DAC_SetDistance(uint16_t distance)
{
    uint16_t dac_value;

    // LED indicador
    if (distance < 20){
        GPIO_SetPinState(PORT_0, PIN_22, 0);
    }
    else{
    	GPIO_SetPinState(PORT_0, PIN_22, 1);
    }

    // Saturación de rango útil
    if (distance <= 5)
    {
        dac_value = 1023;
        GPIO_SetPinState(PORT_0, PIN_1, 1);
        //GPIO_SetPinState(PORT_0, PIN_1, 0);
    }
    else if (distance >= 100)
    {
        dac_value = 0;
        GPIO_SetPinState(PORT_0, PIN_1, 0);
        //GPIO_SetPinState(PORT_0, PIN_1, 1);
    }
    else
    {
    	//GPIO_SetPinState(PORT_0, PIN_1, 1);
    	GPIO_SetPinState(PORT_0, PIN_1, 0);
        dac_value = ((100 - distance) * 1023) / (100 - 5);
    }

    DAC_UpdateValue(dac_value);
}
