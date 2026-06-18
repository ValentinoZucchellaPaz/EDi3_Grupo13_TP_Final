#include "uart_config.h"

volatile uint8_t uart_rx_cmd = 0;
static uint8_t header[2] = {0xAA, 0x55};

void UART0_Config(void)
{
    UART_PinConfig(UART_TX0_P0_2);
    UART_PinConfig(UART_RX0_P0_3);

    UART_CFG_T uartCfg =
        {
            .baudRate = 115200,
            .dataBits = UART_DBITS_8,
            .parity = UART_PARITY_NONE,
            .stopBits = UART_STOPBIT_1};

    UART_FIFO_CFG_T fifoCfg =
        {
            .resetRxBuf = ENABLE,
            .resetTxBuf = ENABLE,
            .dmaMode = ENABLE,
            .level = UART_FIFO_TRGLEV0};

    UART_Init(UART0, &uartCfg);
    UART_FIFOConfig(UART0, &fifoCfg);
    UART_TxEnable(UART0);

    UART_IntConfig(UART0, UART_INT_RBR, ENABLE); // Habilita interrupciones por UART
    NVIC_EnableIRQ(UART0_IRQn);
    DMA_Init();
}

void UART0_SendString(char *str)
{
    while (*str)
    {
        UART_Send(UART0, (uint8_t *)str, 1, BLOCKING);
        str++;
    }
}

void UART0_SendBuffer(uint8_t *buffer, uint32_t size)
{
    UART_Send(UART0, header, 2, BLOCKING);
    DMA_SendBuffer(buffer, size);
}

void UART0_IRQHandler(void)
{
    // falta limpiar banderas?
    uint8_t dato;

    if (UART_Receive(UART0, &dato, 1, NONE_BLOCKING))
    {
        uart_rx_cmd = dato;

        switch (dato)
        {
        case 'A':
            Servo_SetModo(SERVO_MODO_AUTO);
            break;

        case 'M':
            Servo_SetModo(SERVO_MODO_MANUAL);
            break;

        default:
            break;
        }
    }
}
