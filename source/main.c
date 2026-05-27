#include "bsp_gpio.h"
#include "bsp_sys.h"
#include "ddl.h"
#include "drv_input.h"
#include "drv_rtc.h"
#include "led_panel.h"
#include "lpm.h"

#define SLEEP_TIME 1500

void sys_init(void);

volatile uint8_t check_self_Start = 0;
volatile uint8_t second = 0;
volatile uint8_t minute = 0;
volatile uint8_t hour = 0;
// 在ui app 中定义

static volatile uint8_t IGN_ON_OFF = 0; // 电门
static volatile uint8_t rtc_time_1s_flag = 0;
static volatile uint8_t last_ign_state = 0xFF; // 用于记录电门上一次的状态，以捕捉动作瞬间
static volatile uint16_t DeepSleep_cnt = 0;

int32_t main(void)
{
	sys_init();

	while (1)
	{
		if (IGN_ON_OFF != last_ign_state) // 边沿触发逻辑
		{
			if (IGN_ON_OFF == 1)
			{
				// 电门打开：上电
				(void)Bsp_Gpio_Write(BspGpioIdPower, TRUE);
				check_self_Start = 0;
			}
			else
			{
				// 电门关掉：断电、清除状态
				if (last_ign_state == 1)
				{
				}

				(void)Bsp_Gpio_Write(BspGpioIdPower, FALSE);
			}
			last_ign_state = IGN_ON_OFF;
		}

		if (IGN_ON_OFF)
		{
			if (check_self_Start == 0)
			{
				static volatile uint32_t last_check_time = 0;
				if (BSP_SYS_GetTickMs() - last_check_time >= 10) // 10ms
				{
					last_check_time = BSP_SYS_GetTickMs();

					// 自检函数 定义 并更改check_self_Start状态
				}
			}
			else
			{
				// 自检结束后，进入正常显示逻辑
			}
		}
		else
		{
			if (DeepSleep_cnt >= SLEEP_TIME)
			{
				// 关闭ADC，IO口设置为模拟输入，关闭非必要中断
				BSP_WDT_Feed();
				// 在即将休眠的最后一刻，确认一下电门是关闭
				// 避免在清理中断挂起期间用户刚好开电门，导致睡死且无法被边沿中断唤醒。
				if (FALSE == DRV_Input_ReadRaw(DrvInputIdIgn))
				{
					Lpm_GotoDeepSleep(FALSE); //=-=oopc
				}
				// ==========================================
				// 此时系统沉睡，直到 RTC 周期中断 或 外部电门引脚中断 唤醒
				// ==========================================
				// 醒来第一件事：立刻恢复系统高速时钟(休眠会自动切回低速主频)
				Sysctrl_SetRCHTrim(SysctrlRchFreq16MHz);	  //=-=oopc
				Sysctrl_ClkSourceEnable(SysctrlClkRCH, TRUE); //=-=oopc
				Sysctrl_SysClkSwitch(SysctrlClkRCH);		  //=-=oopc
				// 恢复所有外设和 GPIO 状态
				// 恢复引脚数字功能，唤醒并稳定 BGR 和 ADC

				BSP_WDT_Feed(); // 醒来立刻喂狗
			}
		}

		if (rtc_time_1s_flag == 1)
		{
			rtc_time_1s_flag = 0;

			//=-=oopc
			stc_rtc_time_t readtime;
			if (Ok == DRV_RTC_ReadDateTime(&readtime))
			{
				second = BCD2DEC(readtime.u8Second);
				minute = BCD2DEC(readtime.u8Minute);
				hour = BCD2DEC(readtime.u8Hour);
			}
		}

		// 1s喂狗
		static volatile uint32_t last_wdt_cnt = 0;
		if (BSP_SYS_GetTickMs() - last_wdt_cnt >= 1000)
		{
			last_wdt_cnt = BSP_SYS_GetTickMs();
			BSP_WDT_Feed();
		}
	}
}

void sys_init(void)
{
	BSP_SysTick_Init();
	(void)Bsp_Gpio_Init();
	(void)DRV_Input_Init();
	(void)DRV_RTC_Init(12, 0); // 明确忽略返回值
	LedPanel_Init();
	BSP_WDT_Init();
}

void SysTick_IRQHandler(void)
{
	// 1ms,一次
	BSP_SYS_TickInc();
	DRV_Input_Task1ms();

	// 电门开关检测
	if (TRUE == DRV_Input_IsActive(DrvInputIdIgn))
	{
		IGN_ON_OFF = 1;
		DeepSleep_cnt = 0;
	}
	else
	{
		IGN_ON_OFF = 0;
		if (DeepSleep_cnt < SLEEP_TIME)
			DeepSleep_cnt++;
	}
}

// RTC 中断服务函数
void Rtc_IRQHandler(void)
{
	if (Rtc_GetPridItStatus() == TRUE)
	{
		Rtc_ClearPrdfItStatus();
		rtc_time_1s_flag = 1;
	}
	// 清除闹钟标志，防止死锁
	if (Rtc_GetAlmfItStatus() == TRUE)
	{
		Rtc_ClearAlmfItStatus();
	}
}
