/**
 * TouchGFX 板级配置 — 硬件初始化 + HAL 实例创建
 * ===============================================
 * hw_init():       GPIO、时钟、SDRAM、LTDC 等硬件初始化 (由 main.c 完成)
 * touchgfx_init(): 创建 TouchGFXHAL 实例, 设置 framebuffer 于 0xD0000000
 *                  使用 NoDMA (后续可替换为 STM32H7DMA 启用 DMA2D 加速)
 *
 * 调用顺序: main.c → TouchGFX_Init() → hw_init() → touchgfx_init() → GUITask
 */
#include <touchgfx/hal/BoardConfiguration.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/OSWrappers.hpp>
#include <touchgfx/hal/NoDMA.hpp>
#include <platform/driver/lcd/LCD16bpp.hpp>

#include "TouchGFXHAL.hpp"
#include "STM32H7TouchController.hpp"
#include <gui/common/FrontendHeap.hpp>

/* Frame buffer in SDRAM at 0xD0000000 (32MB SDRAM base) */
#define FRAMEBUFFER_ADDR 0xD0000000

static touchgfx::NoDMA             dma;
static STM32H7TouchController      touchController;
static TouchGFXHAL*                hal_ptr = nullptr;

namespace touchgfx
{
    void hw_init()
    {
        /* All hardware init is done in main.c before calling this:
         *   sys_cache_enable(), HAL_Init(), sys_stm32_clock_init(192,5,2,4),
         *   delay_init(480), MX_DMA_Init(), MX_GPIO_Init(), MX_I2C2_Init(),
         *   MX_TIM3_Init(), MX_DMA2D_Init(), mpu_memory_protection(), sdram_init()
         */
    }

    void touchgfx_init()
    {
        static touchgfx::LCD16bpp lcd;

        hal_ptr = new TouchGFXHAL(dma, lcd, touchController, 800, 480);

        hal_ptr->initialize();
        hal_ptr->setFrameBufferStartAddresses((void*)FRAMEBUFFER_ADDR, NULL, NULL);

        FrontendHeap::getInstance();
    }
}

TouchGFXHAL& touchgfx_hal_instance()
{
    return *hal_ptr;
}

/* ---- C-callable wrappers for main.c ---- */

extern "C" {

void TouchGFX_Init(void)
{
    touchgfx::hw_init();
    touchgfx::touchgfx_init();
}

#include "FreeRTOS.h"
#include "task.h"

#define GUI_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define GUI_TASK_STACK    (4096)

static void GUITask(void* params)
{
    (void)params;

    hal_ptr->taskEntry();
}

void TouchGFX_StartTask(void)
{
    xTaskCreate(GUITask, "GUITask",
                GUI_TASK_STACK,
                NULL,
                GUI_TASK_PRIORITY,
                NULL);
}

} /* extern "C" */
