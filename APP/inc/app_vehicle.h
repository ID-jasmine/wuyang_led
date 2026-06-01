#ifndef __APP_VEHICLE_H__
#define __APP_VEHICLE_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	void App_Vehicle_ResetSelfCheck(void);
	boolean_t App_Vehicle_SelfCheckTask10ms(void);
	void App_Vehicle_NotifyRtcTick500ms(void);
	void App_Vehicle_Task10ms(void);

#ifdef __cplusplus
}
#endif

#endif
