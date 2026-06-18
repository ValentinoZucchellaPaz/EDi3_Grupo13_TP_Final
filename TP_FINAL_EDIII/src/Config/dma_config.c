#include "dma_config.h"

volatile uint8_t dma_busy = 0;

void DMA_Init(void)
{
    GPDMA_Init();
    NVIC_EnableIRQ(DMA_IRQn);
}

void DMA_SendBuffer(uint8_t *buffer, uint32_t size)
{
    if (dma_busy)
        return;

    dma_busy = 1;

    GPDMA_Endpoint_T srcc = {
        .width = GPDMA_BYTE,
        .burst = GPDMA_BSIZE_1,
        .increment = ENABLE
    };

    GPDMA_Endpoint_T dstc = {
        .width = GPDMA_BYTE,
        .burst = GPDMA_BSIZE_1,
        .increment = DISABLE
    };

    GPDMA_Channel_CFG_T dmaCfg = {0};

    dmaCfg.channelNum   = GPDMA_CH_0;
    dmaCfg.transferSize = size;
    dmaCfg.type         = GPDMA_M2P;
    dmaCfg.srcMemAddr   = (uint32_t)buffer;
    dmaCfg.dstConn      = GPDMA_UART0_Tx;
    dmaCfg.src          = srcc;
    dmaCfg.dst          = dstc;
    dmaCfg.intTC        = ENABLE;
    dmaCfg.intErr       = ENABLE;

    Status st = GPDMA_SetupChannel(&dmaCfg);

    if (st != SUCCESS)
    {
        dma_busy = 0;
        return;
    }

    GPDMA_ChannelStart(GPDMA_CH_0);
}

void DMA_IRQHandler(void)
{
	//UART0_SendString("DMA IRQ\r\n");
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0))
    {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
        dma_busy = 0;
    }

    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0))
    {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_0);
        dma_busy = 0;
    }
}
