/**
 * Cortex-M7 Enhanced Fault Handler
 * ================================
 * Saves complete fault context to a no-init struct preserved across resets.
 * Inspect crash_info in debugger after hitting the infinite loop.
 *
 * Stacked frame on exception entry (MSP or PSP):
 *   [0] r0,  [1] r1,  [2] r2,  [3] r3,
 *   [4] r12, [5] lr,  [6] pc,  [7] xpsr
 */

#include <stdint.h>
#include <stddef.h>

/* ---- Crash info struct — NOLOAD so it survives resets ---- */
typedef struct {
    /* Fault status registers (offset 0-16) */
    uint32_t cfsr;       /* 0xE000ED28 */
    uint32_t hfsr;       /* 0xE000ED2C */
    uint32_t mmfar;      /* 0xE000ED34 */
    uint32_t bfar;       /* 0xE000ED38 */
    uint32_t afsr;       /* 0xE000ED3C */

    /* SP at exception entry (offset 20) */
    uint32_t exc_sp;

    /* Stacked register frame — 8 words pushed by CPU (offset 24-55) */
    uint32_t stacked_r0;
    uint32_t stacked_r1;
    uint32_t stacked_r2;
    uint32_t stacked_r3;
    uint32_t stacked_r12;
    uint32_t stacked_lr;
    uint32_t stacked_pc;
    uint32_t stacked_xpsr;

    /* EXC_RETURN and MSP/PSP at handler entry (offset 56-68) */
    uint32_t fault_lr;
    uint32_t saved_msp;
    uint32_t saved_psp;

    /* Valid flag (offset 72) */
    uint32_t valid;
} crash_info_t;

/* Placed in .crash section (NOLOAD in linker), survives warm reset */
__attribute__((section(".crash"), used)) crash_info_t crash_info;

/* ---- Naked HardFault handler ---- */
__attribute__((naked))
void HardFault_Handler(void)
{
    __asm volatile (
        /* Save EXC_RETURN for later */
        "mov    r2, lr                    \n"

        /* Get SP that was in use at fault time */
        "tst    lr, #4                    \n"
        "itt    eq                        \n"
        "mrseq  r0, msp                   \n"
        "beq    1f                        \n"
        "mrs    r0, psp                   \n"
        "1:                               \n"
        /* r0 = exception frame address */

        "ldr    r12, =crash_info          \n"

        /* Save current MSP, PSP, EXC_RETURN, and fault SP */
        "mrs    r3, msp                   \n"
        "str    r3, [r12, %[saved_msp]]   \n"
        "mrs    r3, psp                   \n"
        "str    r3, [r12, %[saved_psp]]   \n"
        "str    r2, [r12, %[fault_lr]]    \n"
        "str    r0, [r12, %[exc_sp]]      \n"

        /* ---- Read SCB fault registers BEFORE copying stacked frame ---- */
        "ldr    r3, =0xE000ED28           \n"
        "ldr    r4, [r3, #0]             \n"  /* CFSR */
        "ldr    r5, [r3, #4]             \n"  /* HFSR */
        "ldr    r6, [r3, #12]            \n"  /* MMFAR */
        "ldr    r1, [r3, #16]            \n"  /* BFAR */
        "ldr    r2, [r3, #20]            \n"  /* AFSR */

        /* Store SCB registers at offsets 0-16 */
        "str    r4, [r12, %[cfsr_off]]   \n"
        "str    r5, [r12, %[hfsr_off]]   \n"
        "str    r6, [r12, %[mmfar_off]]  \n"
        "str    r1, [r12, %[bfar_off]]   \n"
        "str    r2, [r12, %[afsr_off]]   \n"

        /* Copy stacked r0-r3 from exception frame to crash_info.stacked_r0+ */
        "add    r12, r12, %[sr0_off]     \n"
        "ldmia  r0!, {r3-r6}             \n"  /* r3=s.r0, r4=s.r1, r5=s.r2, r6=s.r3 */
        "stmia  r12!, {r3-r6}            \n"

        /* Copy stacked r12, lr, pc, xpsr */
        "ldmia  r0!, {r3-r6}             \n"  /* r3=s.r12, r4=s.lr, r5=s.pc, r6=s.xpsr */
        "stmia  r12!, {r3-r6}            \n"

        /* Set valid flag */
        "ldr    r12, =crash_info          \n"
        "ldr    r3, =0xBEEFCAFE           \n"
        "str    r3, [r12, %[valid_off]]  \n"

        /* Infinite loop */
        "b       .                        \n"
        :
        : [cfsr_off]    "i" (offsetof(crash_info_t, cfsr)),
          [hfsr_off]    "i" (offsetof(crash_info_t, hfsr)),
          [mmfar_off]   "i" (offsetof(crash_info_t, mmfar)),
          [bfar_off]    "i" (offsetof(crash_info_t, bfar)),
          [afsr_off]    "i" (offsetof(crash_info_t, afsr)),
          [saved_msp]   "i" (offsetof(crash_info_t, saved_msp)),
          [saved_psp]   "i" (offsetof(crash_info_t, saved_psp)),
          [fault_lr]    "i" (offsetof(crash_info_t, fault_lr)),
          [exc_sp]      "i" (offsetof(crash_info_t, exc_sp)),
          [valid_off]   "i" (offsetof(crash_info_t, valid)),
          [sr0_off]     "i" (offsetof(crash_info_t, stacked_r0))
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r12", "memory"
    );
}

/* MemManage/BusFault/UsageFault — escalate to HardFault for unified capture */
__attribute__((naked))
void MemManage_Handler(void)
{
    __asm volatile ("b HardFault_Handler");
}

__attribute__((naked))
void BusFault_Handler(void)
{
    __asm volatile ("b HardFault_Handler");
}

__attribute__((naked))
void UsageFault_Handler(void)
{
    __asm volatile ("b HardFault_Handler");
}
