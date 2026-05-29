/**
 * TouchGFX HAL — STM32H747 LTDC 800x480 RGB565 单帧缓冲
 * ========================================================
 * 负责: LTDC 配置、帧缓冲管理、VSYNC 同步、DMA2D 阻塞拷贝
 * 帧缓冲: 0xD0000000 (SDRAM), 800x480x16bpp
 */
#ifndef TOUCHGFXHAL_HPP
#define TOUCHGFXHAL_HPP

#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/DMA.hpp>
#include <touchgfx/lcd/LCD.hpp>

class TouchGFXHAL : public touchgfx::HAL
{
public:
    TouchGFXHAL(touchgfx::DMA_Interface& dma,
                touchgfx::LCD& lcd,
                touchgfx::TouchController& tc,
                uint16_t width, uint16_t height);

    void initialize();
    void taskEntry() override;

    void disableInterrupts() override;
    void enableInterrupts() override;
    void configureInterrupts() override;
    void enableLCDControllerInterrupt() override;
    void flushFrameBuffer(const touchgfx::Rect& rect) override;
    bool blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes) override;
    void registerEventListener(touchgfx::UIEventListener& listener) override;

protected:
    /**
     * Override tick() to validate listener before HAL::tick() dereferences it.
     * If the listener was corrupted (e.g. by heap overflow), restores a safe
     * dummy listener to prevent HardFault.
     */
    void tick() override;

    uint16_t* getTFTFrameBuffer() const override;
    void setTFTFrameBuffer(uint16_t* addr) override;

private:
    void lockDMAToHardware();
    void unlockDMAToHardware();

    uint16_t* framebuffer;
    bool dmaLocked;
};

#endif
