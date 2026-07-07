#ifndef __ADC_CPLD_H
#define __ADC_CPLD_H
/*
 * CM7 适配 stub：shared_config.h 用 #include "adc_cpld.h" 取 CHANNEL_SAMPLES
 * 算 SHM_DCMI_A/B/CTRL 地址。CM7 不跑 ADC/DCMI(CM4 的事),这里只给常量。
 *
 * 真定义在 CM4/Core/Inc/adc_cpld.h,两边值必须一致(改一边要同步)。
 * 当前 CHANNEL_SAMPLES = 256 * 60 = 15360(SHM_DCMI 区 ~15KB×2 + ctrl)。
 */
#define SAMPLES_PER_LINE   256u
#define LINES_PER_FRAME    60u
#define CHANNEL_SAMPLES    (SAMPLES_PER_LINE * LINES_PER_FRAME)

#endif /* __ADC_CPLD_H */
