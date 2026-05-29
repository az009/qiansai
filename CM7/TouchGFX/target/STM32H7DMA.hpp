/**
 * DMA2D 加速器 — STM32H7 Chrom-ART
 * ================================
 * 通过 HAL_DMA2D 实现硬件 Blit，支持:
 *   - R2M (单色填充)
 *   - M2M (内存到内存拷贝)
 *   - M2M_PFC (带像素格式转换的拷贝)
 * 句柄: 复用 Hardware/ltdc.c 的 g_dma2d_handle
 */
#ifndef STM32H7DMA_HPP
#define STM32H7DMA_HPP

#include <touchgfx/hal/DMA.hpp>

class STM32H7DMA : public touchgfx::DMA_Interface
{
public:
    STM32H7DMA();
    virtual ~STM32H7DMA();

    touchgfx::BlitOperations getBlitCaps() override;
    void setupDataCopy(const touchgfx::BlitOp& blitOp) override;
    void setupDataFill(const touchgfx::BlitOp& blitOp) override;
    void signalDMAInterrupt() override;
};

#endif
