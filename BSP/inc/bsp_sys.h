#ifndef __BSP_SYS_H
#define __BSP_SYS_H

#include "ddl.h"

void BSP_SysTick_Init(void);
void BSP_SYS_TickInc(void);
uint32_t BSP_SYS_GetTickMs(void);
void BSP_SYS_DelayMs(uint32_t ms);
void BSP_WDT_Init(void);
void delay100us_safe(uint32_t u32Cnt);

#endif
