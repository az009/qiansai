#include "pin_switch.h"
#include "main.h"

/* 拓展板两片 MUX(U1/U2)各受 A0/A1/EN 3 脚控制,2 位选信号选 4 种协议引脚路由。
 * U1 管"发"线、U2 管"收"线(或类似分组),两片同设置。
 * 移植自队友 desk-win/Protocol_Analysis。引脚宏(U1_A0_Pin 等)在 main.h(CubeMX 生成)。*/
void Select_Pin(Pin_Select a){
    switch (a) {
        case UART_Pin:
            HAL_GPIO_WritePin(U1_A0_GPIO_Port, U1_A0_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U1_A1_GPIO_Port, U1_A1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U1_EN_GPIO_Port, U1_EN_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_A0_GPIO_Port, U2_A0_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_A1_GPIO_Port, U2_A1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U2_EN_GPIO_Port, U2_EN_Pin, GPIO_PIN_SET);
        break;

        case I2C_Pin:
            HAL_GPIO_WritePin(U1_A0_GPIO_Port, U1_A0_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U1_A1_GPIO_Port, U1_A1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U1_EN_GPIO_Port, U1_EN_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_A0_GPIO_Port, U2_A0_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_A1_GPIO_Port, U2_A1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_EN_GPIO_Port, U2_EN_Pin, GPIO_PIN_SET);
        break;

        case SPI_Pin:
            HAL_GPIO_WritePin(U1_A0_GPIO_Port, U1_A0_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U1_A1_GPIO_Port, U1_A1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U1_EN_GPIO_Port, U1_EN_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_A0_GPIO_Port, U2_A0_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U2_A1_GPIO_Port, U2_A1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_EN_GPIO_Port, U2_EN_Pin, GPIO_PIN_SET);
        break;

        case CAN_Pin:
            HAL_GPIO_WritePin(U1_A0_GPIO_Port, U1_A0_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U1_A1_GPIO_Port, U1_A1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U1_EN_GPIO_Port, U1_EN_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(U2_A0_GPIO_Port, U2_A0_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U2_A1_GPIO_Port, U2_A1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U2_EN_GPIO_Port, U2_EN_Pin, GPIO_PIN_SET);
        break;

        case NONE_Pin:
            HAL_GPIO_WritePin(U1_EN_GPIO_Port, U1_EN_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(U2_EN_GPIO_Port, U2_EN_Pin, GPIO_PIN_RESET);
        break;
    }
}

/* active_proto 1=UART,2=SPI,3=I2C,4=CAN → Pin_Select。越界(0/>4)→ NONE 断开。
 * 三处调用:数据屏切录制协议(defaultTask) / Settings Apply / 开机 config-load。*/
void Select_Pin_ByProto(uint8_t active_proto){
    static const Pin_Select pmap[5] = {NONE_Pin, UART_Pin, SPI_Pin, I2C_Pin, CAN_Pin};
    Select_Pin((active_proto >= 1U && active_proto <= 4U) ? pmap[active_proto] : NONE_Pin);
}
