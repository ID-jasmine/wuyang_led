#include "system_runtime.h"

#include "board_config.h"
#include "bsp_gpio.h"
#include "bsp_lpm.h"
#include "bsp_sys.h"
#include "dev_speed_rpm.h"
#include "drv_adc.h"
#include "drv_button.h"
#include "drv_eeprom.h"
#include "drv_input.h"
#include "drv_rtc.h"
#include "led_panel.h"
#include "product_config.h"

en_result_t SystemRuntime_Init(void)
{
	en_result_t result = Ok;

	if (Ok != Board_ConfigValidate())
	{
		return ErrorInvalidParameter;
	}

	BSP_SysTick_Init();
	if (Ok != Bsp_Gpio_Init())
	{
		result = Error;
	}

#if (PRODUCT_FEATURE_EEPROM != 0u)
	if (Ok != DRV_EEPROM_Init())
	{
		result = Error;
	}
#endif

	if (Ok != DRV_Input_Init())
	{
		result = Error;
	}
	DRV_Button_Init();
	if (Ok != DRV_RTC_Init(12u, 0u))
	{
		result = Error;
	}

#if (PRODUCT_FEATURE_SPEED_RPM != 0u)
	if (Ok != DEV_SpeedRpm_Init())
	{
		result = Error;
	}
#endif

#if (PRODUCT_FEATURE_LED_PANEL != 0u)
	if (Ok != LedPanel_Init())
	{
		result = Error;
	}
#endif

#if (PRODUCT_FEATURE_ADC != 0u)
	DRV_ADC_Init();
#endif

	BSP_WDT_Init();
	return result;
}

void SystemRuntime_Task1ms(void)
{
#if (PRODUCT_FEATURE_SPEED_RPM != 0u)
	DEV_SpeedRpm_Task1ms();
#endif
	DRV_Input_Task1ms();
#if (PRODUCT_FEATURE_ADC != 0u)
	DRV_ADC_Task1ms();
#endif
	DRV_Button_Task1ms();
}

void SystemRuntime_PrepareDeepSleep(void)
{
#if (PRODUCT_FEATURE_ADC != 0u)
	DRV_ADC_DeInit();
#endif
	(void)Bsp_Gpio_InitSleepPins();
	BSP_WDT_Feed();
}

void SystemRuntime_RestoreClockAfterWake(void)
{
	BSP_LPM_RestoreClockAfterWakeup();
	BSP_WDT_Feed();
}

void SystemRuntime_WakeupIgnCheck(void)
{
#if (PRODUCT_FEATURE_ADC != 0u)
	DRV_ADC_WakeupIgnCheck();
#endif
}

void SystemRuntime_ResumeAfterIgn(void)
{
	(void)Bsp_Gpio_Init();
#if (PRODUCT_FEATURE_SPEED_RPM != 0u)
	(void)DEV_SpeedRpm_Init();
#endif
#if (PRODUCT_FEATURE_EEPROM != 0u)
	(void)DRV_EEPROM_Init();
#endif
#if (PRODUCT_FEATURE_ADC != 0u)
	DRV_ADC_Wakeup();
#endif
}

void SystemRuntime_AbortIgnCheck(void)
{
#if (PRODUCT_FEATURE_ADC != 0u)
	DRV_ADC_DeInit();
#endif
}
