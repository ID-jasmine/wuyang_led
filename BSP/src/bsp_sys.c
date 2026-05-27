#include "bsp_sys.h"
#include "flash.h"
#include "wdt.h"

static volatile uint32_t s_u32SysTickMs = 0u;

void BSP_SysTick_Init(void)
{
	stc_sysctrl_clk_cfg_t stcCfg;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralFlash, TRUE);
	Flash_WaitCycle(FlashWaitCycle0);
	Sysctrl_SetRCHTrim(SysctrlRchFreq16MHz);

	stcCfg.enClkSrc = SysctrlClkRCH;
	stcCfg.enHClkDiv = SysctrlHclkDiv1;
	stcCfg.enPClkDiv = SysctrlPclkDiv1;
	Sysctrl_ClkInit(&stcCfg);

	s_u32SysTickMs = 0u;
	SysTick_Config(16000); // 16MHz / 16000 = 1ms
}

void BSP_SYS_TickInc(void)
{
	s_u32SysTickMs++;
}

uint32_t BSP_SYS_GetTickMs(void)
{
	return s_u32SysTickMs;
}

void BSP_SYS_DelayMs(uint32_t ms)
{
	uint32_t start_ms;

	start_ms = BSP_SYS_GetTickMs();
	while ((BSP_SYS_GetTickMs() - start_ms) < ms)
	{
	}
}

void BSP_WDT_Init(void)
{
	Sysctrl_SetPeripheralGate(SysctrlPeripheralWdt, TRUE);
	Wdt_Init(WdtResetEn, WdtT6s55);
	Wdt_Start();
	Wdt_Feed();
}

void BSP_WDT_Feed(void)
{
	Wdt_Feed();
}

void delay100us_safe(uint32_t u32Cnt)
{
	uint32_t ticks_per_100us = 1600;
	uint32_t t0, t1;

	while (u32Cnt-- > 0)
	{
		t0 = SysTick->VAL;
		while (1)
		{
			t1 = SysTick->VAL;
			if (t0 < t1)
			{
				if ((t0 + (SysTick->LOAD - t1)) >= ticks_per_100us)
				{
					break;
				}
			}
			else
			{
				if ((t0 - t1) >= ticks_per_100us)
				{
					break;
				}
			}
		}
	}
}
