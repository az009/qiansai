/**
 * OS 包装器 — FreeRTOS + CMSIS_RTOS_V2
 * =====================================
 * VSYNC 使用 FreeRTOS Task Notification (比 CMSIS_V2 消息队列在 ISR 场景更可靠)
 * Framebuffer 信号量继续使用 CMSIS_V2 (仅在 task context 使用)
 *
 * 来源: TouchGFX 4.26.1 touchgfx/os/OSWrappers_cmsis.cpp (已适配 CMSIS_V2)
 */
#include <touchgfx/hal/OSWrappers.hpp>
#include <touchgfx/hal/GPIO.hpp>
#include <touchgfx/hal/HAL.hpp>

#include <assert.h>
#include <cmsis_os2.h>

#include "FreeRTOS.h"
#include "task.h"

using namespace touchgfx;

static osSemaphoreId_t frame_buffer_sem = NULL;
static TaskHandle_t vsync_task_handle = NULL;

void OSWrappers::initialize()
{
    frame_buffer_sem = osSemaphoreNew(1, 1, NULL);

    osSemaphoreAcquire(frame_buffer_sem, osWaitForever);
}

void OSWrappers::takeFrameBufferSemaphore()
{
    assert(frame_buffer_sem);
    osSemaphoreAcquire(frame_buffer_sem, osWaitForever);
}

void OSWrappers::giveFrameBufferSemaphore()
{
    assert(frame_buffer_sem);
    osSemaphoreRelease(frame_buffer_sem);
}

void OSWrappers::tryTakeFrameBufferSemaphore()
{
    assert(frame_buffer_sem);
    osSemaphoreAcquire(frame_buffer_sem, 0);
}

void OSWrappers::giveFrameBufferSemaphoreFromISR()
{
    assert(frame_buffer_sem);
    osSemaphoreRelease(frame_buffer_sem);
}

void OSWrappers::signalVSync()
{
    if (vsync_task_handle)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(vsync_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void OSWrappers::waitForVSync()
{
    vsync_task_handle = xTaskGetCurrentTaskHandle();

    ulTaskNotifyTake(pdTRUE, 0);           /* drain any stale notification */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* wait for next VSYNC */
}

void OSWrappers::taskDelay(uint16_t ms)
{
    osDelay(static_cast<uint32_t>(ms));
}

/* ---- FreeRTOS hook: MCU load measurement (optional) ---- */

extern "C"
{
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
    {
        (void)xTask;
        (void)pcTaskName;
        while (1);
    }

    void vApplicationMallocFailedHook(void)
    {
        while (1);
    }

    void vApplicationIdleHook(void)
    {
        /* MCU load: setMCUActive(false) when idle task runs */
        touchgfx::HAL::getInstance()->setMCUActive(false);
    }
}
