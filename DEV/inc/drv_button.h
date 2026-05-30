#ifndef __DRV_BUTTON_H__
#define __DRV_BUTTON_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		DrvButtonIdK1 = 0u,
		DrvButtonIdK2,
		DrvButtonIdBoth,
		DrvButtonIdMax,
	} en_drv_button_id_t;

	typedef enum
	{
		DrvButtonEventNone = 0u,
		DrvButtonEventShortPress,
		DrvButtonEventLongPress,
	} en_drv_button_event_t;

	void DRV_Button_Init(void);
	void DRV_Button_Task1ms(void);
	en_drv_button_event_t DRV_Button_GetEvent(en_drv_button_id_t id);
	boolean_t DRV_Button_IsTimeout10s(void);
	void DRV_Button_ClearTimeout(void);

#ifdef __cplusplus
}
#endif

#endif
