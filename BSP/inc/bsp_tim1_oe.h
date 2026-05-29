#ifndef __BSP_TIM1_OE_H__
#define __BSP_TIM1_OE_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	en_result_t BSP_Tim1Oe_Init(uint8_t brightness_percent);
	en_result_t BSP_Tim1Oe_SetBrightness(uint8_t brightness_percent);
	uint8_t BSP_Tim1Oe_GetBrightness(void);

#ifdef __cplusplus
}
#endif

#endif
