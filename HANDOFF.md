# 千赛项目结构调整交接

更新时间:2026-08-14
分支:`codex/restructure-project`
基点:`bare-metal-stable`(9aeac82)

## 当前状态

结构重构和分模块构建验证已完成,未改变协议采集、双核通信、TouchGFX UI 和 SD 录制/回放的运行逻辑。

已完成:

- 从 Git 索引移除 `CM7/Core/example` 厂商参考工程;本地目录和 2029 个参考文件仍保留,由 `.gitignore` 忽略。
- `shared_buf.h`、`shared_config.h` 收敛到 `Common/Inc`,CM4/CM7/TouchGFX 共用同一份。
- CM4 协议代码移动到 `CM4/Modules/Protocols/{UART,SPI,I2C,CAN,Timing}`。
- 新增 `tools/verify.ps1`,支持从零配置、清理重编、检查共享头唯一性、检查 CM4 模块编译位置和双核 ELF 产物。
- 更新 `工程解析.md` 的模块路径和一键验证命令。

## 关键决策

- 保留 `CM4`、`CM7` 顶层目录,避免破坏 CubeMX `.ioc`、TouchGFX Designer 和 linker 的预期路径。
- 不修改 `mx-generated.cmake`;自定义 include 路径放在可维护的 `CM4/CMakeLists.txt` 和 `CM7/CMakeLists.txt`。
- 厂商示例只从 Git 索引移除,不删除本地文件,保留调试参考价值。
- 验证构建放在 `build/verify/`,不污染日常开发用的 `CM4/build`、`CM7/build`。

## 验证

最终验证命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1 -Clean
```

验证项:

- Git 中共享头文件只剩 `Common/Inc/shared_buf.h`、`Common/Inc/shared_config.h`。
- CM4 五个协议/计时模块源文件存在,并从新路径参与编译。
- CM4 从零配置并清理重编,生成 `build/verify/CM4/qiansai_CM4.elf`。
- CM7 从零配置并清理重编,生成 `build/verify/CM7/qiansai_CM7.elf`。

最新一次脚本验证通过。CM7 仍有原有编译警告(`lcd.c` char 下标、TouchGFX 成员初始化顺序、未使用变量/函数),非本次结构改动引入。

## 下一步

1. 在 GitHub 审查 `codex/restructure-project` 分支。
2. 合并前烧录 `build/verify/CM4/qiansai_CM4.elf` 和 `build/verify/CM7/qiansai_CM7.elf`,做板上四协议和 UI 冒烟测试。
3. 测试通过后合并回目标分支,再删除本地 `CM7/Core/example` 参考目录释放约 501.5MB 磁盘空间。

## 注意事项

- `tools/verify.ps1` 需要 STM32CubeCLT 的 `cmake`、`ninja`、`arm-none-eabi-gcc` 在 PATH 中;也可用参数显式指定 CMake/Ninja 路径。
- PowerShell 5.1 会把部分 CMake 进度输出当成 stderr;脚本已按退出码判断结果,不要只看红色文字。
- CubeMX 重新生成后仍需按 `工程解析.md` 第 8 节检查 TouchGFX init、task stack、MPU region、ffconf、sd_diskio 和 SPI CS 电平。
