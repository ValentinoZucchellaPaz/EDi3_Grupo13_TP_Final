#include "uart_config.h"

// ── Flags públicos ───────────────────────────────────────
volatile uint8_t cmd_recibido = 0;
volatile uint8_t ultimo_cmd   = 0;

// ── Buffer de trama — static, vive siempre en memoria ───
// El DMA apunta acá, NO puede estar en el stack
static uint8_t trama[TRAMA_SIZE];

// ── Canal DMA y LLI ─────────────────────────────────────
static int8_t          dma_canal = -1;
static GPDMA_LLI_T     lli_uart  __attribute__((aligned(4)));

// ────────────────────────────────────────────────────────
// Función interna: busca canal DMA libre (CH7 → CH0)
// ────────────────────────────────────────────────────────
static int8_t buscarCanalDMA(void){
    int8_t ch;
    for(ch = 7; ch >= 0; ch--){
        if(GPDMA_IntGetStatus(GPDMA_STAT_ENABLED_CH, ch) == RESET){
            return ch;
        }
    }
    return -1;
}

// ────────────────────────────────────────────────────────
void configUART(void){

    // ── 1. Configurar pines con la nueva función ────────
    // UART_PinConfig maneja PINSEL internamente
    // TX: P0.2, RX: P0.3 → UART0
    UART_PinConfig(UART_TX0_P0_2);
    UART_PinConfig(UART_RX0_P0_3);

    // ── 2. Configurar parámetros de comunicación ────────
    UART_CFG_T uartCfg = {
        .baudRate = UART_BAUD_RATE,
        .dataBits = UART_DBITS_8,
        .parity   = UART_PARITY_NONE,
        .stopBits = UART_STOPBIT_1
    };

    // ── 3. Configurar FIFO ──────────────────────────────
    UART_FIFO_CFG_T fifoCfg = {
        .resetRxBuf = ENABLE,           // limpiar FIFO RX al arrancar
        .resetTxBuf = ENABLE,           // limpiar FIFO TX al arrancar
        .dmaMode    = ENABLE,           // habilitar DMA para TX
        .level      = UART_FIFO_TRGLEV0 // trigger cada 1 byte recibido
    };

    UART_Init(LPC_UART0, &uartCfg);
    UART_FIFOConfig(LPC_UART0, &fifoCfg);

    // ── 4. Habilitar transmisor ─────────────────────────
    // En los nuevos drivers es UART_TxEnable, no UART_TxCmd
    UART_TxEnable(LPC_UART0);

    // ── 5. Habilitar IRQ de recepción ───────────────────
    UART_IntConfig(LPC_UART0, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART0_IRQn);

    // ── 6. Buscar canal DMA disponible ──────────────────
    // GPDMA_Init() debe llamarse en main() antes de esto
    dma_canal = buscarCanalDMA();
}

// ────────────────────────────────────────────────────────
void uartEnviarMedicion(uint16_t angulo,
                        uint16_t distancia,
                        uint8_t  modo){

    // Si no hay canal libre, no mandamos nada
    if(dma_canal < 0) return;

    // Esperar a que el DMA termine la trama anterior
    // para no pisar datos en vuelo
    while(GPDMA_IntGetStatus(GPDMA_STAT_ENABLED_CH, dma_canal) == SET);

    // ── Armar trama ──────────────────────────────────────
    uint8_t chk = (uint8_t)((angulo    >> 8) & 0xFF)
                ^ (uint8_t)( angulo          & 0xFF)
                ^ (uint8_t)((distancia >> 8) & 0xFF)
                ^ (uint8_t)( distancia       & 0xFF)
                ^ modo;

    trama[0] = TRAMA_STX;
    trama[1] = (uint8_t)((angulo    >> 8) & 0xFF);
    trama[2] = (uint8_t)( angulo          & 0xFF);
    trama[3] = (uint8_t)((distancia >> 8) & 0xFF);
    trama[4] = (uint8_t)( distancia       & 0xFF);
    trama[5] = modo;
    trama[6] = chk;
    trama[7] = TRAMA_ETX;

    // ── Configurar DMA M2P: trama[] → UART0 TX ──────────
    GPDMA_Channel_CFG_T dmaCfg = {
        .channelNum   = (uint8_t)dma_canal,
        .transferSize = TRAMA_SIZE,
        .type         = GPDMA_M2P,
        .srcMemAddr   = (uint32_t)trama,
        .dstMemAddr   = 0,
        .srcConn      = 0,
        .dstConn      = GPDMA_CONN_UART0_Tx,
        .src = {
            .width     = GPDMA_WIDTH_BYTE,  // 8 bits por dato
            .burst     = GPDMA_BSIZE_1,     // de a 1 byte
            .increment = ENABLE             // avanza en trama[]
        },
        .dst = {
            .width     = GPDMA_WIDTH_BYTE,
            .burst     = GPDMA_BSIZE_1,
            .increment = DISABLE            // UART: dirección fija
        },
        .intTC      = DISABLE,
        .intErr     = DISABLE,
        .linkedList = 0                     // one-shot, sin ciclo
    };

    GPDMA_SetupChannel((uint8_t)dma_canal, &dmaCfg);
    GPDMA_ChannelStart((uint8_t)dma_canal);
}

// ────────────────────────────────────────────────────────
// Handler de recepción: llega un comando desde la PC
// ────────────────────────────────────────────────────────
void UART0_IRQHandler(void){
    uint32_t iir = UART_GetIntId(LPC_UART0);

    // Verificar que la IRQ es por dato recibido (RDA)
    if((iir & UART_IIR_INTID_MASK) == UART_IIR_INTID_RDA){
        // Leer el byte recibido — limpiar el flag automáticamente
        uint8_t byte = UART_ReceiveByte(LPC_UART0);

        // Solo seteamos flags — la lógica va en while(1)
        ultimo_cmd   = byte;
        cmd_recibido = 1;
    }
}