#ifndef __PIN_SWITCH_H
#define __PIN_SWITCH_H

#include "stm32h7xx_hal.h"

/* 拓展板协议引脚 MUX(U1/U2 两片,各 A0/A1/EN 控制)选哪个协议通到外部接口。
 * 移植自队友 desk-win/Protocol_Analysis 的 CM7/Core/Hardware/pin_switch.c。
 * Apply 切协议 / 开机 config-load 时调,和 CM4 外设重配同步。*/
typedef enum {
    UART_Pin,
    I2C_Pin,
    SPI_Pin,
    CAN_Pin,
    NONE_Pin   /* MUX 断开(EN=0) */
} Pin_Select;

#ifdef __cplusplus
extern "C" {
#endif
void Select_Pin(Pin_Select a);
#ifdef __cplusplus
}
#endif

#endif /* __PIN_SWITCH_H */
