# CM4 RTOS 移植交接(2026-07-08)

> 接手人:这个 branch(`m4-rtos-port`)在做 CM4 RTOS 完整移植,已完成代码移植 + 编译通过 + RTOS 存活验证,但卡在 HSEM 配置 IPC 链不通。下面是完整状态。

## 目标

把队友 `CM4/core/example/partner/Protocol_Analysis-main/` 的 **CM4 RTOS 完整架构**移植过来,替代之前有多个 bug 的裸机 main 循环。融合:
- **CM4**:队友 FreeRTOS(`Proto_Select` task 调度 + 每协议 `Callback_Task` 收外部信号)
- **CM7**:我们的 TouchGFX 显示(`waveWidget` 读 shm head-6 画波形)

## 背景(为什么要重做)

之前裸机移植有多个 bug,根因是漏了/改错了队友逻辑:
- I2C `task_done` dead flag(全代码无人设 1,数据烂在 ring)
- UART 自环 `HAL_UART_Transmit` 阻塞持有 `huart->Lock`,与 DMA 接收回调重启 DMA 的 `__HAL_LOCK` 冲突 → DMA 停
- shm 残留(`shm_init` 不清 data[])→ "幻觉波形"

决定推翻,基于队友最新 RTOS 代码重新移植(spec: [docs/superpowers/specs/2026-07-08-m4-rtos-port-design.md](superpowers/specs/2026-07-08-m4-rtos-port-design.md),plan: [docs/superpowers/plans/2026-07-08-m4-rtos-port.md](superpowers/plans/2026-07-08-m4-rtos-port.md))。

## 架构

```
外部信号 → MUX(CM7 Select_Pin_ByProto) → H7 外设引脚 → CM4 外设收
    → Callback_Task(收数据)→ shm_push 裸字节 → CM7 waveWidget 读 head-6 显示

配置 IPC: CM7 Settings Apply → 写 SHM_CONFIG → shm_config_notify(Take+Release HSEM_ID_CONFIG=1)
    → CM4 HSEM ISR (HAL_HSEM_FreeCallback) → xSemaphoreGiveFromISR(hsem_config_sem)
    → Proto_Select task Take sem → apply_xxx_config_from_shm + 建 Callback_Task
```

关键接口(双核必须一致):
- `shared_buf.h`:队友超集(ring head/tail/data[2048] @ 0x30000000 + push_u16/u32/buf 工具)
- `shared_config.h`:队友扩展版(proto_config_t:can 多 tx_id/filter、i2c 多 own_mode、+dcmi_enable;CFG_MAGIC 升 CFG2)
- `HSEM_ID_CONFIG=1` / `HSEM_ID_DONE=2`

## 已完成(Phase 1-6,全部 commit 到 m4-rtos-port)

| Phase | 内容 | commit |
|-------|------|--------|
| 1 | branch m4-rtos-port(基于 bare-lcd 383ad36) | — |
| 2 | shared headers 替换(CM4+CM7):shared_buf.h 队友超集 + shared_config.h 队友扩展版 + CFG_MAGIC CFG1→CFG2 + 修注释 SRAM4→SRAM1 | e56419c, 4932ecd |
| 3 | 搬队友文件到 CM4:freertos.c + my_uart/i2c/spi/can_check.c(+.h) + adc_cpld + my_dma_catch + my_dwt_count + usart_printf + stm32h7xx_hal_timebase_tim + stm32h7xx_it + FreeRTOSConfig.h;CMakeLists.txt 加 4 source | 3f33dc9, 6a1b197, 474a93c, b1fad8f, 5c596e2 |
| 4 | CM4 main.c 改造(启动 RTOS + 删裸机分发);Callback_Task push 改裸字节(4 协议去 magic/统计) | 3027115, 3fd771e |
| 5 | CM7 适配(Settings 字段名一致,不用改 UI) | — |
| 6 | CM4 + CM7 编译通过(CM4 100568B/FLASH 9.6%, CM7 762592B/72.7%) | 0dfb254 |

**编译适配(必要 stub)**:
- `my_dma_catch.c`:`#if 0` 禁用硬件(TIM1+DMA1_Stream3 没配),保留函数签名链接
- `stm32h7xx_it.c`:注释 `hdma_dcmi/hdma_tim1_up` extern + 清空未启用的 DMA ISR
- `CM7/Core/Inc/adc_cpld.h` 新建 stub(只 CHANNEL_SAMPLES 常量)

## 上板验证进度(Phase 7)

- ✅ **Step 2 RTOS 存活**:**过 192 tick 不卡死!** 队友的 `stm32h7xx_hal_timebase_tim.c`(HAL timebase 走 TIM 不走 SysTick)解了之前裸机 RTOS 实验的 SysTick 卡死。**这是整个移植的最大风险点,已通过**
- ❌ Step 3+ 卡住:HSEM 配置 IPC 链不通

## 当前卡点:HSEM 配置 IPC 不通

**现象**:
- 开机 Proto_Select task 创建+跑(`[CM4] Proto_Select: started, waiting sem`)
- **Apply 配置后,CM4 没收到 HSEM 通知**(没 woke,没 reconfig OK printf)
- 波形固定(shm 没新数据 → waveWidget 读 SRAM 残留)

**代码层面已查(都对)**:
- NVIC HSEM2 启用(`HAL_MspInit` → `HAL_NVIC_EnableIRQ(HSEM2_IRQn)`)
- ISR 链:`HSEM2_IRQHandler` → `HAL_HSEM_IRQHandler` → `HAL_HSEM_FreeCallback`(`freertos.c:345`)
- `HAL_HSEM_FreeCallback`: `xSemaphoreGiveFromISR(hsem_config_sem)`
- `Proto_Select`: `xSemaphoreTake(hsem_config_sem)`
- CM7 `shm_config_notify`(`main.c:120`): `HAL_HSEM_Take/Release(HSEM_ID_CONFIG)`
- Settings Apply 调 `shm_config_notify`(`Settings_ScreenView.cpp:275`)

**奇怪矛盾现象(未解释)**:
- 之前用 `xSemaphoreTake(sem, 1000)` timeout 版本:开机 woke **几十次**(busy loop),但 `isr_count` printf 显示 2(可能 `uart1_printf` 不支持 `%lu` 读错)
- 改成 `xSemaphoreTake(sem, portMAX_DELAY)`:Apply 后**完全没 woke**
- 两者矛盾(busy loop vs 不动)

**工作区未 commit 改动**(诊断用):
- `CM4/Core/Src/freertos.c`:
  - Proto_Select 栈 256→512(之前栈溢出 HardFault,增大后不卡)
  - 加 `g_hsem_isr_count`(callback ISR 触发计数)
  - Proto_Select timeout 打印 `isr=X, cm7_notify=Y`(对比两边计数)
- `CM7/Core/Src/main.c`:
  - `shm_config_notify` 加 `SHM_STATUS->reserved[0]++`(cm7_notify 计数,CM4 读)

## 下一步(接手继续诊断)

**已编译带诊断的 elf(CM4 + CM7),烧双核 + Apply,看每秒打印**:
```
[CM4] wait: isr=X, cm7_notify=Y
```
**Apply 前后对比两个数**:
- `cm7_notify` 涨 + `isr` 不涨 → **HSEM Release 没触发 CM4 ISR**(HSEM 配置问题,虽然代码看着对 —— 可能 HSEM ID/mask/ProcID/NVIC 优先级)
- `cm7_notify` 涨 + `isr` 也涨 → HSEM 链通了,问题在 sem/task(可能 uart1_printf %lu 读错导致误判)
- `cm7_notify` 不涨 → **CM7 Apply 根本没调 shm_config_notify**(Settings Apply 按钮→applyConfig 链断)

## 关键信息

- **branch**: `m4-rtos-port`(本地,没 push 过)
- **回滚点**: `9aeac82`(Settings 自动刷新,裸机稳定版;回滚会丢 MUX/ADC 硬件)
- **最新 commit**: `0dfb254`(编译通过;之后工作区有诊断改动未 commit)
- **spec**: [docs/superpowers/specs/2026-07-08-m4-rtos-port-design.md](superpowers/specs/2026-07-08-m4-rtos-port-design.md)
- **plan**: [docs/superpowers/plans/2026-07-08-m4-rtos-port.md](superpowers/plans/2026-07-08-m4-rtos-port.md)
- **队友代码源**: `CM4/core/example/partner/Protocol_Analysis-main/CM4/Core/{Src,Inc}/`

## 待清理(HSEM 解决后)

- 诊断 printf + `g_hsem_isr_count` + `SHM_STATUS->reserved[0]` 计数器(`freertos.c` + `main.c`)→ 验证后拿掉
- Proto_Select 栈 512(确认 256 够再恢复,不够保留)
- `my_dma_catch.c` #if 0 stub → CubeMX 配 TIM1+DMA1_Stream3 后启用
- `stm32h7xx_it.c` 注释的 hdma extern → CubeMX 配 DCMI DMA 后恢复
- `CM7/Core/Inc/adc_cpld.h` stub → CubeMX 配 ADC 后用 CM4 完整版

## 范围外(本次不做)

- ADC 完整功能(adc_cpld + NanoEdge AI):硬件已配,代码 stub
- CAN 高级 UI(tx_id/filter):proto_config_t 字段已搬,UI 留默认
- shm push 带统计格式(B 方案):设计预留,以后改
- MUX 验证(Step 6):HSEM 解决后再测

## 已知风险

- `uart1_printf` 可能不支持 `%lu`(之前 isr_count=2 可能读错实际值)—— 诊断时用 `%u` 或 hex 更稳
- FreeRTOS heap 15360(多个 512 栈 task,可能紧张)—— 加 task 注意 heap
- HSEM ID 冲突:队友加 `HSEM_ID_DONE=2`(我们用 0=boot, 1=config,2 应该空闲)
