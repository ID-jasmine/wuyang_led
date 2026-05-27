#include "bsp_rtc.h"
#include "bsp_sys.h"
#include "wdt.h"

void BSP_RTC_Init(uint8_t hour, uint8_t minute)
{
	stc_rtc_initstruct_t stcRtcInit;
	uint32_t start_ms;

	Sysctrl_ClkSourceEnable(SysctrlClkXTL, TRUE);
	Sysctrl_SetPeripheralGate(SysctrlPeripheralRtc, TRUE);

	start_ms = BSP_SYS_GetTickMs();
	while ((BSP_SYS_GetTickMs() - start_ms) < 500u)
	{
		Wdt_Feed();
	}

	stcRtcInit.rtcAmpm = RtcPm;
	stcRtcInit.rtcClksrc = RtcClkXtl;
	stcRtcInit.rtcPrdsel.rtcPrdsel = RtcPrdx;
	stcRtcInit.rtcPrdsel.rtcPrdx = 1u;
	stcRtcInit.rtcPrdsel.rtcPrds = RtcNone;

	stcRtcInit.rtcTime.u8Second = 0u;
	stcRtcInit.rtcTime.u8Minute = DEC2BCD(minute);
	stcRtcInit.rtcTime.u8Hour = DEC2BCD(hour);
	stcRtcInit.rtcTime.u8DayOfWeek = 1u; // 星期一
	stcRtcInit.rtcTime.u8Day = 1u;		 // 1日
	stcRtcInit.rtcTime.u8Month = 1u;	 // 1月
	stcRtcInit.rtcTime.u8Year = 0u;		 // 00年

	stcRtcInit.rtcCompen = RtcCompenEnable;
	stcRtcInit.rtcCompValue = 0u;

	Rtc_Init(&stcRtcInit);
	Rtc_Cmd(TRUE);

	Rtc_ClearPrdfItStatus();
	Rtc_ClearAlmfItStatus();
	Rtc_AlmIeCmd(TRUE);
	EnableNvic(RTC_IRQn, IrqLevel3, TRUE);
}

en_result_t BSP_RTC_ReadDateTime(stc_rtc_time_t *time)
{
	return Rtc_ReadDateTime(time);
}

en_result_t BSP_RTC_SetTime(const stc_rtc_time_t *time)
{
	return Rtc_SetTime((stc_rtc_time_t *)time);
}
