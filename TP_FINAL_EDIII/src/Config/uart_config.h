#ifndef UART_CONFIG_H_
#define UART_CONFIG_H_

#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "dma_config.h"
#include "servo_pwm.h"

extern volatile uint8_t uart_rx_cmd;

void UART0_Config(void);
void UART0_SendString(char *str);
void UART0_SendBuffer(uint8_t *buffer, uint32_t size);

#endif
