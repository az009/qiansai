/**
 * DMA2D 加速器 — STM32H7 Chrom-ART via HAL
 * =========================================
 * getBlitCaps(): 返回 BLIT_OP_FILL + BLIT_OP_COPY
 * setupDataCopy(): M2M or M2M_PFC
 * setupDataFill():  R2M
 */
#include "STM32H7DMA.hpp"
#include "stm32h7xx_hal.h"

extern DMA2D_HandleTypeDef g_dma2d_handle;

static touchgfx::DMA_Queue dma_queue;

STM32H7DMA::STM32H7DMA()
    : touchgfx::DMA_Interface(dma_queue)
{
}

STM32H7DMA::~STM32H7DMA() = default;

touchgfx::BlitOperations STM32H7DMA::getBlitCaps()
{
    return static_cast<touchgfx::BlitOperations>(
        touchgfx::BLIT_OP_FILL |
        touchgfx::BLIT_OP_FILL_WITH_ALPHA |
        touchgfx::BLIT_OP_COPY
    );
}

void STM32H7DMA::setupDataCopy(const touchgfx::BlitOp& blitOp)
{
    uint32_t srcAddr  = (uint32_t)blitOp.pSrc;
    uint32_t dstAddr  = (uint32_t)blitOp.pDst;

    /* For RGB565: 2 bytes per pixel */
    uint32_t srcWidth = blitOp.nSteps * 2;
    uint32_t dstWidth = (blitOp.nLoops > 0)
        ? ((blitOp.dstLoopStride > 0) ? blitOp.dstLoopStride : srcWidth)
        : srcWidth;

    g_dma2d_handle.Init.Mode         = DMA2D_M2M;
    g_dma2d_handle.Init.ColorMode    = DMA2D_OUTPUT_RGB565;
    g_dma2d_handle.Init.OutputOffset = (dstWidth - srcWidth) / 2;

    HAL_DMA2D_Init(&g_dma2d_handle);

    g_dma2d_handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
    g_dma2d_handle.LayerCfg[1].InputOffset    = 0;
    g_dma2d_handle.LayerCfg[1].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
    g_dma2d_handle.LayerCfg[1].InputAlpha     = 0xFF;

    HAL_DMA2D_ConfigLayer(&g_dma2d_handle, 1);

    HAL_DMA2D_Start(&g_dma2d_handle, srcAddr, dstAddr, blitOp.nSteps, blitOp.nLoops);
    HAL_DMA2D_PollForTransfer(&g_dma2d_handle, 10);
}

void STM32H7DMA::setupDataFill(const touchgfx::BlitOp& blitOp)
{
    uint32_t dstAddr  = (uint32_t)blitOp.pDst;
    uint32_t fillColor = blitOp.color;

    uint32_t dstWidth = (blitOp.nLoops > 0)
        ? ((blitOp.dstLoopStride > 0) ? blitOp.dstLoopStride : (blitOp.nSteps * 2))
        : (blitOp.nSteps * 2);

    g_dma2d_handle.Init.Mode         = DMA2D_R2M;
    g_dma2d_handle.Init.ColorMode    = DMA2D_OUTPUT_RGB565;
    g_dma2d_handle.Init.OutputOffset = (dstWidth - (blitOp.nSteps * 2)) / 2;

    HAL_DMA2D_Init(&g_dma2d_handle);

    HAL_DMA2D_Start(&g_dma2d_handle, fillColor, dstAddr, blitOp.nSteps, blitOp.nLoops);
    HAL_DMA2D_PollForTransfer(&g_dma2d_handle, 10);
}

void STM32H7DMA::signalDMAInterrupt()
{
    /* Polling mode — no ISR needed */
}
