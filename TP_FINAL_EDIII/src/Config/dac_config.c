#include "dac_config.h"

void DAC_Config(void) {
    DAC_Init();
    DAC_SetBias(DAC_350uA);
    DAC_UpdateValue(0);
}


void DAC_SetDistance(uint16_t distance) {
    if (distance < 20)
	{
		GPIO_SetPinState(PORT_0, 22, 0);
		DAC_UpdateValue(1023);
	}
	else
	{
		GPIO_SetPinState(PORT_0, 22, 1);
		DAC_UpdateValue(0);
	}
}

