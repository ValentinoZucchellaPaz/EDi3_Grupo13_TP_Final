#ifndef UART_CONFIG_H_
#define UART_CONFIG_H_

#include "LPC17xx.h"
#include "lpc17xx_uart.h"

void UART0_Config(void);
void UART0_SendString(char *str);

#endif
