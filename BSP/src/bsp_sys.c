#include "bsp_sys.h"
#include "flash.h"
#include "wdt.h"

static volatile uint32_t s_u32SysTickMs = 0u;

/**
 * @brief  初始化系统节拍
 * @details 配置系统时钟为16MHz，并将SysTick设置为1ms中断节拍。
 */
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

/**
 * @brief  系统毫秒节拍自增
 * @details 通常在SysTick中断服务函数中调用。
 */
void BSP_SYS_TickInc(void)
{
	s_u32SysTickMs++;
}

/**
 * @brief  获取系统毫秒计数
 * @return 自初始化以来累计的毫秒数
 */
uint32_t BSP_SYS_GetTickMs(void)
{
	return s_u32SysTickMs;
}

/**
 * @brief  毫秒级阻塞延时
 * @param  [in] ms 延时时长，单位ms
 */
void BSP_SYS_DelayMs(uint32_t ms)
{
	uint32_t start_ms;

	start_ms = BSP_SYS_GetTickMs();
	while ((BSP_SYS_GetTickMs() - start_ms) < ms)
	{
	}
}

/**
 * @brief  初始化看门狗
 * @details 使能WDT，配置为复位模式，并立即执行一次喂狗。
 */
void BSP_WDT_Init(void)
{
	Sysctrl_SetPeripheralGate(SysctrlPeripheralWdt, TRUE);
	Wdt_Init(WdtResetEn, WdtT6s55);
	Wdt_Start();
	Wdt_Feed();
}

/**
 * @brief  喂狗
 */
void BSP_WDT_Feed(void)
{
	Wdt_Feed();
}

/**
 * @brief  100us单位的安全阻塞延时
 * @param  [in] u32Cnt 延时次数，每次约100us
 * @details 基于SysTick当前计数值实现，适合短时间精细延时。
 */
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
