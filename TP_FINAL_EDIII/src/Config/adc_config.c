#include "adc_config.h"

void ADC_Config(void)
{
    ADC_Init(200000); // 200 kHz

    ADC_PinConfig(ADC_CHANNEL_0);

    ADC_BurstDisable();
}

uint16_t ADC_Read(void)
{
    ADC_StartCmd(ADC_START_NOW);

    while (ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE) == RESET);

    return ADC_ChannelGetData(ADC_CHANNEL_0);
}
