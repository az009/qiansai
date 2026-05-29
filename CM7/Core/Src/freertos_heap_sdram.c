/**
 * FreeRTOS heap placed in SDRAM (0xD0100000) to free DTCM for application use.
 *
 * configAPPLICATION_ALLOCATED_HEAP=1 delegates ucHeap definition to this file,
 * so heap_4.c references our section-placed array instead of a static one.
 */
#include "FreeRTOS.h"

__attribute__((section(".sdram_heap"), aligned(8)))
uint8_t ucHeap[configTOTAL_HEAP_SIZE];
