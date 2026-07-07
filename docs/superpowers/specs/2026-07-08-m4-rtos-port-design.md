# CM4 RTOS 完整移植设计

**日期**: 2026-07-08
**分支**: `m4-rtos-port`(基于当前 `bare-lcd` 383ad36 + 未提交改动)
**回滚点**: `9aeac82`(Settings 自动刷新,已知稳定)

## 背景

当前 CM4 裸机 main 循环移植有多个 bug:
- I2C `task_done` dead flag(全代码无人设 1,数据烂在 ring)
- UART 自环发送 `HAL_UART_Transmit` 阻塞持有 `huart->Lock`,与 DMA 接收回调重启 DMA 的 `__HAL_LOCK` 冲突 → DMA 停 → 后续不接收
- shm 残留(`shm_init` 只清 head/tail 不清 data)→ "幻觉波形"

根因:裸机移植时漏了/改错了队友的协议逻辑。

**两个已验证可行的事实**:
1. 我们的 CM7 显示链路(waveWidget 读 shm head-6 显示)在裸机 UART 自环时验证过可行
2. 队友的 RTOS 代码(`partner/Protocol_Analysis-main`)经验证能接收外部信号(UART/SPI/I2C/CAN),只是没接显示

**目标**:融合二者 —— CM4 用队友 RTOS 完整架构(接收外部信号),CM7 保持我们的显示。

## 决策总结

| 项 | 决策 |
|---|---|
| base | 当前工作区(`bare-lcd` 383ad36 + 未提交改动,MUX/ADC 硬件保留)|
| branch | `m4-rtos-port`(基于 base)|
| 回滚点 | `9aeac82`(Settings 自动刷新,移植失败 `git reset` 到此)|
| CM4 | 队友 RTOS 完整架构 |
| CM7 | 保持我们显示(TouchGFX/Settings/MUX/SD 录制)|
| shm 接口 | 裸字节(CM4 push 裸数据,CM7 waveWidget 读 head-6 不变);设计预留以后改"带统计格式"的扩展空间 |
| 配置 IPC | `HSEM_ID_CONFIG=1`(两边一致,CM7 Release 不变)|
| shared_config.h | 用队友的(扩展版 proto_config_t,CM7 改 include,Settings UI 暂不加新字段)|

## 架构

```
外部信号 → MUX(CM7 Select_Pin_ByProto) → H7 外设引脚 → CM4 外设收
                                                              ↓
                          CM4 FreeRTOS: Proto_Select task 调度
                            → 每协议一个 Callback_Task(收数据)
                                                              ↓ shm_push 裸字节
                          共享内存 SRAM1 ring (0x30000000)
                                                              ↓ read head-6
                          CM7 TouchGFX waveWidget 显示 + SD 录制

配置: CM7 Settings Apply → SHM_CONFIG 写 → Release HSEM_ID_CONFIG(=1)
      → CM4 HSEM ISR Give hsem_config_sem → Proto_Select Take → apply_xxx + 建 Callback_Task
```

**关键双核接口(必须两边一致)**:
- `shared_buf.h`:ring 结构 `head/tail/data[2048]` @ 0x30000000(队友超集,工具函数兼容)
- `shared_config.h`:`proto_config_t` @ 0x30001000(队友扩展版,can 多字段/i2c own_mode/dcmi_enable)
- `HSEM_ID_CONFIG=1` / `HSEM_ID_DONE=2`(队友新增,确认 ID=2 空闲)

## CM4 文件改动

### 替换成队友原版(直接覆盖)

| 文件 | 说明 |
|------|------|
| `CM4/Core/Src/freertos.c` | Proto_Select task + 创建/删除 Callback_Task + HAL_HSEM_FreeCallback(ISR Give sem)|
| `CM4/Core/Src/my_uart_check.c` | UART 协议逻辑 + UART_Callback_Task |
| `CM4/Core/Src/my_i2c_check.c` | I2C 协议逻辑 + I2C_Callback_Task |
| `CM4/Core/Src/my_spi_check.c` | SPI 协议逻辑 + SPI_Callback_Task |
| `CM4/Core/Src/my_can_check.c` | CAN 协议逻辑 + CAN_Callback_Task |
| `CM4/Core/Src/my_dma_catch.c` + `my_dwt_count.c` | 队友辅助 |
| `CM4/Core/Src/usart_printf.c` + `Inc/usart_printf.h` | printf |
| `CM4/Core/Src/adc_cpld.c` + `Inc/adc_cpld.h` | shared_config.h 依赖(`#include "adc_cpld.h"`)|
| `CM4/Core/Src/stm32h7xx_hal_timebase_tim.c` | **解 SysTick 卡死关键**:HAL timebase 走 TIM 不走 SysTick |
| `CM4/Core/Inc/shared_buf.h` | 队友超集(ring 兼容 + push_u16/u32/float/buf/pop_buf 工具函数)|
| `CM4/Core/Inc/shared_config.h` | 队友扩展版 proto_config_t |
| `CM4/Core/Src/stm32h7xx_it.c` | HSEM ISR 等(适配队友)|
| `CM4/Core/Inc/FreeRTOSConfig.h` | 队友 RTOS 配置(对齐 SDRAM heap 布局,见风险)|

### 改造(不替换,改 USER CODE)

**`CM4/Core/Src/main.c`**:
- USER CODE 2 保留外设 init(`My_UART_Init` / `My_I2C_Init` / `My_SPI_Init` / CAN / DWT / HSEM activate / `shm_init`),跟队友 main.c 对齐
- USER CODE WHILE:`osKernelInitialize` + `MX_FREERTOS_Init` + `osKernelStart` 启动 RTOS
- **删除**裸机协议分发逻辑(全部移到 freertos.c task):
  - `apply_uart/i2c/spi/can_config_from_shm`(移到 Proto_Select task)
  - main 循环的 `if(active_proto==N)` 分支(ring 读 + shm_push,移到 Callback_Task)
  - `HAL_HSEM_FreeCallback`(移到 freertos.c,ISR Give sem)
  - `g_config_pending` flag(RTOS 用 sem 替代)
  - 诊断 printf + 自环发送(已确认去掉)

### Callback_Task push 部分改(裸字节适配,A 方案)

队友原版 4 个 Callback_Task 的 push 是带格式:
```c
shm_push(0xFD); shm_push(0xAA);          // magic
shm_push_u16(rx_frame_size);              // 长度
shm_push_u32(total_interval);             // 统计
shm_push_u32(success_count);
shm_push_u32(error_count);
shm_push(ERROR_WINDOW_SIZE);
shm_push_buf(data, rx_frame_size);        // 数据
shm_push_buf(error_history, ERROR_WINDOW_SIZE);
```

**改成只 push 裸数据字节**:
```c
for (uint32_t i = 0; i < rx_frame_size; i++) shm_push(data[i]);
```

4 个协议 task(UART/SPI/I2C/CAN)都改。统计逻辑保留计算(以后改 B 方案带格式时用),但不 push。

## CM7 文件改动

| 操作 | 文件 | 说明 |
|------|------|------|
| 替换 | `CM7/Core/Inc/shared_buf.h` `shared_config.h` | 跟 CM4 一致(队友版);shared_config.h 里 **`CFG_MAGIC` 从 `CFG1`(0x31474643)升 `CFG2`**,proto_config_t 结构变 → 旧 config.bin 自动判失效 |
| 改 | `CM7/TouchGFX/gui/src/settings_screen_screen/Settings_ScreenView.cpp` | proto_config_t 结构跟着变(can 多 tx_id/filter 字段、i2c 多 own_mode);**UI 暂不加新字段**(留默认)|
| 改 | `CM7/Core/Src/freertos.c`(录制 defaultTask)| 如果引用 `SHM_CONFIG` 字段名变了,跟着改(预期不影响:active_proto/uart/spi 字段名一致)|
| **不动** | waveWidget / data_screen / pin_switch / SD 录制逻辑 | 全保留 |

## 验证步骤(每步上板测通再下一步)

| Step | 验证点 | 通过标准 |
|------|--------|---------|
| **0** | `git branch m4-rtos-port` + 备份当前 shared headers | 准备 |
| **1** | 搬完所有队友文件 + 改 main/Callback_Task + CM7 适配 | CM4/CM7 编译通过 |
| **2** | **RTOS 存活**(最关键门槛)| 烧 CM4,UART1 printf 持续打印,等过 ~192 tick(之前卡死点)还活着。**过不了就回滚** |
| **3** | **UART**(先这个)| 数据屏选 UART,外部 USB-TTL 发数据 → 屏幕显示波形(核心链路)|
| **4** | SPI / I2C / CAN 逐个 | 每个接外部信号,屏幕显示正确 framing |
| **5** | 配置 IPC | Settings Apply 切协议/参数 → CM4 Proto_Select 切 task + 重配外设 |
| **6** | MUX | 切协议 → MUX 电平变 + 外部信号经 MUX 到正确外设引脚 |

## 风险 + 对策

| 风险 | 对策 | 兜底 |
|------|------|------|
| **RTOS ~192 tick 卡死**(我们栽过,记忆 cm4-bare-metal.md)| `stm32h7xx_hal_timebase_tim.c` 让 HAL timebase 走 TIM 不走 SysTick;Step 2 先验证存活 | `git reset 9aeac82` 回滚 |
| **shared_config.h 结构变** → 旧 config.bin / 录制 .log 失效 | `CFG_MAGIC` 升 `CFG2`(旧文件自动判失效);录制 header magic 已有检查 | 重新录制 |
| **CM7 Settings 适配新 proto_config_t** | UI 暂不加 can tx_id/filter、i2c own_mode(留默认),只 include 新 header | 以后加 UI |
| **HSEM ID 冲突**:队友加 `HSEM_ID_DONE=2` | 确认 ID=2 空闲(我们只用 0=boot, 1=config);队友 my_*_check.c 用 HSEM_ID_DONE 通知 Proto_Select 自删除 | — |
| **FreeRTOS heap/栈**(我们 SDRAM 迁移过,记忆 sdram_migration_phase1)| 队友 FreeRTOSConfig.h 跟我们 SDRAM 布局对齐(heap 在 SDRAM?);Step 1 编译/链接看 section 错 | 调 FreeRTOSConfig.h |
| **Callback_Task push 改裸字节** 丢统计 | 统计逻辑保留计算不 push,waveWidget 显示裸字节(OK);以后改 B 方案恢复 | — |
| **shared_buf.h 注释还是错的**(SRAM4/D3)| 顺手修注释(SRAM1/D2,跟地址 0x30000000 对齐)| — |

## 回滚策略

- 移植在新 branch `m4-rtos-port` 上做,不动 `bare-lcd`
- Step 2(RTOS 存活)过不了,或任何步骤卡死无法解决 → `git reset --hard 9aeac82`(Settings 自动刷新,已知稳定)
- `9aeac82` 是已存在 commit,不需要额外 tag
- **注意:`9aeac82` 在 `3cbaa10`(MUX+ADC 硬件)之前** —— 回滚到 9aeac82 会丢 MUX/ADC 硬件 + pin_switch。这是"RTOS 方案失败、回到最早稳定裸机点"的代价(用户已确认接受;真回滚时 MUX/ADC 以后重新加)

## 范围外(本次不做)

- ADC(adc_cpld 完整功能 + NanoEdge AI):硬件已配,代码先带着不调用
- CAN 高级配置(tx_id/filter UI):proto_config_t 字段已搬,UI 留默认
- shm push 带统计格式(B 方案):设计预留,以后改
- SPI slave 收外部 master 验证:Step 4 顺带(有外部 master 时)

## 成功标准

1. RTOS 存活过 192 tick 不卡死(Step 2)
2. 外部 UART 信号 → 屏幕显示正确波形(Step 3)
3. 4 个协议都能收外部信号显示(Step 4)
4. Settings Apply 切协议/参数,CM4 重配 + MUX 切对(Step 5/6)
