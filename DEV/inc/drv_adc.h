#ifndef __DRV_ADC_H__
#define __DRV_ADC_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DRV_ADC_RESISTANCE_INVALID_OHM (0xFFFFu)

	typedef enum
	{
		DrvAdcSignalFuel = 0u,
		DrvAdcSignalPowerReference,
		DrvAdcSignalWaterTemp,
		DrvAdcSignalIgn,
		DrvAdcSignalBrightness,
		DrvAdcSignalLeftTurn,
		DrvAdcSignalHighBeam,
		DrvAdcSignalRightTurn,
		DrvAdcSignalCount,
	} en_drv_adc_signal_t;

	void DRV_ADC_Init(void);
	void DRV_ADC_Task1ms(void);
	void DRV_ADC_Task10ms(void);
	boolean_t DRV_ADC_IsReady(void);
	boolean_t DRV_ADC_IsIgnActive(void);
	boolean_t DRV_ADC_IsLeftTurnActive(void);
	boolean_t DRV_ADC_IsRightTurnActive(void);
	boolean_t DRV_ADC_IsHighBeamActive(void);
	boolean_t DRV_ADC_IsIgnLowVoltageActive(void);
	uint16_t DRV_ADC_GetAvg(en_drv_adc_signal_t signal);
	uint16_t DRV_ADC_GetResistanceOhm(en_drv_adc_signal_t signal);
	void DRV_ADC_DeInit(void);
	void DRV_ADC_Wakeup(void);
	void DRV_ADC_WakeupIgnCheck(void);
	boolean_t DRV_ADC_CheckIgnOnce(uint8_t sample_count);

#ifdef __cplusplus
}
#endif

#endif
