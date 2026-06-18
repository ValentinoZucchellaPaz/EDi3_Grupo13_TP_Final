/*
#ifndef DMA_CONFIG_H_
#define DMA_CONFIG_H_

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

void DMA_ConfigSrcMemAddr(uint8_t *buffer);
void DMA_ConfigChannel0();

#endif
*/
#ifndef DMA_CONFIG_H_
#define DMA_CONFIG_H_

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "uart_config.h"

void DMA_Init(void);
void DMA_SendBuffer(uint8_t *buffer, uint32_t size);

#endif
