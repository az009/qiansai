/**
 * TouchGFX HAL — STM32H747 LTDC 800x480 RGB565 单帧缓冲
 * ========================================================
 * LTDC: PLL3R=33.33MHz, 0X4384 面板时序
 * 硬件初始化: ltdc_init() (Core/Hardware/ltdc.c) — 自动面板检测、时钟、复位
 * 帧缓冲: 0xD0000000 (SDRAM 32MB)
 * 同步: LTDC 行中断 → signalVSync()
 */
#include "TouchGFXHAL.hpp"

#include <touchgfx/hal/GPIO.hpp>
#include <touchgfx/hal/OSWrappers.hpp>
#include <touchgfx/UIEventListener.hpp>
#include <touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp>

using namespace touchgfx;

#include "stm32h7xx_hal.h"
extern "C" {
#include "delay.h"
void ltdc_init(void);
}
#include <string.h>

extern LTDC_HandleTypeDef  g_ltdc_handle;
extern DMA2D_HandleTypeDef g_dma2d_handle;

static uint16_t* lcdFrameBuffer = (uint16_t*)0xD0000000;

/* CanvasWidgetRenderer buffer for anti-aliased canvas widgets (Circle, etc.) */
/* Placed in SDRAM after framebuffer (800*480*2 = 0xBB800 bytes) to save DTCM */
#define CANVAS_BUFFER_SIZE 8192
static uint8_t* const canvasBuffer = (uint8_t*)(0xD0000000 + 800*480*2);

/* Dummy listener with all empty handlers — used as fallback when the real
 * listener is corrupted, preventing HardFault in HAL::tick(). */
static touchgfx::UIEventListener s_dummyListener;

/* Debug: record last registered listener addresses for post-mortem inspection.
 * Set a debugger watchpoint on listener_log[N] to break on registration. */
#define LISTENER_LOG_SIZE 8
static uint32_t listener_log[LISTENER_LOG_SIZE];
static uint32_t listener_log_idx = 0;

#define LCD_HEIGHT 480

TouchGFXHAL::TouchGFXHAL(touchgfx::DMA_Interface& dma,
                         touchgfx::LCD& lcd,
                         touchgfx::TouchController& tc,
                         uint16_t width, uint16_t height)
    : touchgfx::HAL(dma, lcd, tc, width, height),
      framebuffer(lcdFrameBuffer),
      dmaLocked(false)
{
}

void TouchGFXHAL::initialize()
{
    HAL::initialize();

    /* Delegate all LTDC hardware init to the proven ltdc_init() in Core/Hardware/ltdc.c.
     * This handles: panel auto-detect → PLL3 clock config → LTDC peripheral init →
     * layer config → panel reset → backlight on.
     */
    ltdc_init();

    CanvasWidgetRenderer::setupBuffer(canvasBuffer, CANVAS_BUFFER_SIZE);

    HAL::registerEventListener(s_dummyListener);
}

void TouchGFXHAL::taskEntry()
{
    enableLCDControllerInterrupt();
    enableInterrupts();

    OSWrappers::waitForVSync();

    backPorchExited();

    HAL::taskEntry();
}

void TouchGFXHAL::disableInterrupts()
{
    __disable_irq();
}

void TouchGFXHAL::enableInterrupts()
{
    __enable_irq();
}

void TouchGFXHAL::configureInterrupts()
{
    NVIC_SetPriority(LTDC_IRQn, 6);
}

void TouchGFXHAL::enableLCDControllerInterrupt()
{
    /* Line interrupt at end of active area (last visible line) */
    LTDC->LIPCR = LCD_HEIGHT - 1;
    __HAL_LTDC_ENABLE_IT(&g_ltdc_handle, LTDC_IT_LI);
    NVIC_EnableIRQ(LTDC_IRQn);
}

void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    /* Single framebuffer — no swap needed, just wait for the rect to be sent */
    HAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    bool disableArt = dmaLocked;
    if (disableArt)
    {
        lockDMAToHardware();
    }
    return HAL::blockCopy(dest, src, numBytes);
}

uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    return framebuffer;
}

void TouchGFXHAL::setTFTFrameBuffer(uint16_t* addr)
{
    framebuffer = addr;
}

void TouchGFXHAL::registerEventListener(touchgfx::UIEventListener& listener)
{
    uint32_t addr = (uint32_t)(&listener);

    /* Log every registration attempt for debugging. Inspect listener_log[]
     * in the debugger to see what addresses are being registered. */
    if (listener_log_idx < LISTENER_LOG_SIZE)
    {
        listener_log[listener_log_idx++] = addr;
    }

    /* Reject obviously bogus pointers — prevents HardFault when HAL::tick()
     * dereferences a stale/corrupted listener. */
    if (addr >= 0xF0000000)
    {
        return; /* Unmapped/reserved range — bogus */
    }
    if (addr < 0x08000000 && addr < 0x20000000)
    {
        return; /* Below Flash and SRAM — bogus */
    }

    HAL::registerEventListener(listener);
}

void TouchGFXHAL::tick()
{
    uint32_t addr = *(uint32_t*)((uint8_t*)this + 96);
    if (addr >= 0xF0000000 || (addr < 0x08000000 && addr < 0x20000000))
    {
        HAL::registerEventListener(s_dummyListener);
    }

    HAL::tick();
}

void TouchGFXHAL::lockDMAToHardware()
{
    if (!dmaLocked)
    {
        dmaLocked = true;
    }
}

void TouchGFXHAL::unlockDMAToHardware()
{
    if (dmaLocked)
    {
        dmaLocked = false;
    }
}

/* ---- LTDC IRQ Handler (VSYNC via Line Interrupt) ---- */

extern "C" void LTDC_IRQHandler(void)
{
    if (__HAL_LTDC_GET_FLAG(&g_ltdc_handle, LTDC_FLAG_LI))
    {
        __HAL_LTDC_CLEAR_FLAG(&g_ltdc_handle, LTDC_FLAG_LI);

        touchgfx::GPIO::toggle(touchgfx::GPIO::VSYNC_FREQ);
        touchgfx::OSWrappers::giveFrameBufferSemaphoreFromISR();
        touchgfx::OSWrappers::signalVSync();
    }
    HAL_LTDC_IRQHandler(&g_ltdc_handle);
}
