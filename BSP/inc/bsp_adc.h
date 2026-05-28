#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum en_bsp_adc_id
	{
		BspAdcIdFuel = 0u,
		BspAdcIdAdPower,
		BspAdcIdWaterTemp,
		BspAdcIdCount,
	} en_bsp_adc_id_t;

	void BSP_ADC_Init(void);
	void BSP_ADC_Start(void);
	void BSP_ADC_Stop(void);
	uint16_t BSP_ADC_GetResult(en_bsp_adc_id_t id);
	void BSP_ADC_DeInit(void);
	void BSP_ADC_Wakeup(void);

#ifdef __cplusplus
}
#endif

#endif
