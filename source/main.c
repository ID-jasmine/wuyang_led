#include "app_vehicle.h"
#include "bsp_gpio.h"
#include "bsp_lpm.h"
#include "bsp_sys.h"
#include "bsp_time_capture.h"
#include "ddl.h"
#include "dev_speed_rpm.h"
#include "drv_adc.h"
#include "drv_button.h"
#include "drv_eeprom.h"
#include "drv_input.h"
#include "drv_rtc.h"
#include "led_panel.h"
#include "product_config.h"
#include "system_runtime.h"

#define SLEEP_TIME PRODUCT_DEEP_SLEEP_DELAY_MS

static volatile uint8_t check_self_Start = 0; // 自检
static volatile uint8_t IGN_ON_OFF = 0;		  // 电门
static volatile uint16_t IGN_CNT = 0;		  // 电门计数器
static volatile uint8_t rtc_time_500ms_flag = 0;
static volatile uint8_t last_ign_state = 0xFF; // 用于记录电门上一次的状态，以捕捉动作瞬间
static volatile uint16_t DeepSleep_cnt = 0;
static volatile uint8_t g_lpm_adc_checking = 0u;

int32_t main(void)
{
	if (Ok != SystemRuntime_Init())
	{
		/* 配置冲突或关键模块初始化失败时保持安全停机。 */
		while (1)
		{
		}
	}

#if (PRODUCT_NORMAL_FUNCTION == 0u)
	(void)DRV_EEPROM_ClearMileageAreas();

	while (1)
	{
		BSP_WDT_Feed();
	}
#else
	while (1)
	{
		if (IGN_ON_OFF != last_ign_state) // 边沿触发逻辑
		{
			if (IGN_ON_OFF == 1)
			{
				// 电门打开：上电
				(void)Bsp_Gpio_Write(BspGpioIdPower, TRUE);
				(void)Bsp_Gpio_Write(BspGpioIdLEDPower, TRUE);

				(void)LedPanel_OutputEnable(FALSE);
				LedPanel_Clear();
				LedPanel_Refresh();
				(void)LedPanel_OutputEnable(TRUE);

				App_Vehicle_ResetSelfCheck();

				check_self_Start = 0;
			}
			else
			{
				// 电门关掉：断电
				(void)LedPanel_OutputEnable(FALSE);
				(void)Bsp_Gpio_Write(BspGpioIdLEDPower, FALSE);
				(void)Bsp_Gpio_Write(BspGpioIdPower, FALSE);
			}

			last_ign_state = IGN_ON_OFF;
		}

		if (IGN_ON_OFF)
		{
			if (0u != rtc_time_500ms_flag)
			{
				rtc_time_500ms_flag = 0u;
				App_Vehicle_NotifyRtcTick500ms();
			}

			if (check_self_Start == 0)
			{
				static volatile uint32_t last_check_time = 0;
				if (BSP_SYS_GetTickMs() - last_check_time >= 10u)
				{
					last_check_time = BSP_SYS_GetTickMs();

					check_self_Start = App_Vehicle_SelfCheckTask10ms();
				}
			}
			else
			{
				static volatile uint32_t last_vehicle_time = 0;
				if (BSP_SYS_GetTickMs() - last_vehicle_time >= 10u)
				{
					last_vehicle_time = BSP_SYS_GetTickMs();

					App_Vehicle_Task10ms();
				}
			}
		}
		else
		{
			if (DeepSleep_cnt >= SLEEP_TIME)
			{
				SystemRuntime_PrepareDeepSleep();
				g_lpm_adc_checking = 1u;

				BSP_LPM_EnterDeepSleep();

				SystemRuntime_RestoreClockAfterWake();

				if (rtc_time_500ms_flag == 1u)
				{
					rtc_time_500ms_flag = 0u;

					SystemRuntime_WakeupIgnCheck();

					if (TRUE == DRV_ADC_CheckIgnOnce(5u))
					{
						SystemRuntime_ResumeAfterIgn();

						IGN_ON_OFF = 1u;
						DeepSleep_cnt = 0u;
						g_lpm_adc_checking = 0u;
					}
					else
					{
						SystemRuntime_AbortIgnCheck();
						IGN_ON_OFF = 0u;
					}
				}
				else
				{
					SystemRuntime_AbortIgnCheck();
				}
			}
		}

		static volatile uint32_t last_wdt_normal_cnt = 0;
		if (BSP_SYS_GetTickMs() - last_wdt_normal_cnt >= 1000u)
		{
			last_wdt_normal_cnt = BSP_SYS_GetTickMs();
			BSP_WDT_Feed();
		}
	}
#endif
}

void SysTick_IRQHandler(void)
{
	// 1ms,一次
	BSP_SYS_TickInc();

	SystemRuntime_Task1ms();

	if (g_lpm_adc_checking)
	{
		return;
	}
	// 电门开关检测
	if (TRUE == DRV_ADC_IsIgnActive())
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
		rtc_time_500ms_flag = 1;
	}
	// 清除闹钟标志，防止死锁
	if (Rtc_GetAlmfItStatus() == TRUE)
	{
		Rtc_ClearAlmfItStatus();
	}
}

void Tim3_IRQHandler(void)
{
	BSP_TimeCapture_IRQHandler();
}
