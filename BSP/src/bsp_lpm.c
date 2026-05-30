#include "bsp_lpm.h"
#include "adc.h"
#include "ddl.h"
#include "lpm.h"
#include "sysctrl.h"

void BSP_LPM_EnterDeepSleep(void)
{
	Lpm_GotoDeepSleep(FALSE);
}

void BSP_LPM_RestoreClockAfterWakeup(void)
{
	Sysctrl_SetRCHTrim(SysctrlRchFreq16MHz);
	Sysctrl_ClkSourceEnable(SysctrlClkRCH, TRUE);
	Sysctrl_SysClkSwitch(SysctrlClkRCH);
}
