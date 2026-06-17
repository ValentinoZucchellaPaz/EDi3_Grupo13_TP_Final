#include "dac_config.h"
#include "lpc17xx_gpio.h"

/** @brief Configura el DAC y pin de salida */
void DAC_Config(void)
{
    DAC_Init();
    DAC_SetBias(DAC_700uA);
    DAC_UpdateValue(0);
}

/** @brief Maneja y actualiza salidas:
 *
 * - LED de proximidad (P0.22): prende con distancia menor 20cm
 *
 * - pin de Buzzer (P0.1): prende con distancia menor 5cm
 *
 * - señal DAC (P0.26): mapea distancia a rango de DAC y saca por ahi
 * */
void DAC_SetDistance(uint16_t distance)
{
    uint16_t dac_value;

    // LED indicador
    if (distance < 20)
        GPIO_SetPinState(PORT_0, PIN_22, 0);
    else
        GPIO_SetPinState(PORT_0, PIN_22, 1);

    // Buzer
    if (distance <= 5)
        GPIO_SetPinState(PORT_0, PIN_1, 1);
    else
        GPIO_SetPinState(PORT_0, PIN_1, 0);

    // Onda DAC - Osciloscopio
    dac_value = ((distance * 1023) / 500); // chequear esto
    DAC_UpdateValue(dac_value);
}
