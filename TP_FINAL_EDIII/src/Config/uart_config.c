#include "uart_config.h"

void UART0_Config(void)
{
    UART_PinConfig(UART_TX0_P0_2);
    UART_PinConfig(UART_RX0_P0_3);

    UART_CFG_T uartCfg =
    {
        .baudRate = 115200,
        .dataBits = UART_DBITS_8,
        .parity   = UART_PARITY_NONE,
        .stopBits = UART_STOPBIT_1
    };

    UART_FIFO_CFG_T fifoCfg =
    {
        .resetRxBuf = ENABLE,
        .resetTxBuf = ENABLE,
        .dmaMode    = ENABLE,
        .level      = UART_FIFO_TRGLEV0
    };

    UART_Init(UART0, &uartCfg);
    UART_FIFOConfig(UART0, &fifoCfg);
    UART_TxEnable(UART0);

    DMA_Init();
}

void UART0_SendString(char *str)
{
    while (*str)
    {
        UART_Send(UART0, (uint8_t*)str, 1, BLOCKING);
        str++;
    }
}

void UART0_SendBuffer(uint8_t *buffer, uint32_t size)
{
    DMA_SendBuffer(buffer, size);
}
