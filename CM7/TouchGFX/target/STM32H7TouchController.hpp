/**
 * Touch 控制器桩 — 暂无触摸屏
 * ============================
 * 后续接入触摸时替换 sampleTouch() 为实际 I2C/SPI 读取逻辑
 */
#ifndef STM32H7TOUCHCONTROLLER_HPP
#define STM32H7TOUCHCONTROLLER_HPP

#include <platform/driver/touch/TouchController.hpp>

class STM32H7TouchController : public touchgfx::TouchController
{
public:
    STM32H7TouchController() {}

    void init() override {}
    bool sampleTouch(int32_t& x, int32_t& y) override
    {
        (void)x;
        (void)y;
        return false;  /* No touch yet */
    }
};

#endif
