#ifndef __BSP_ADC_H__
#define __BSP_ADC_H__

#include "board_config.h"
#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef en_board_adc_id_t en_bsp_adc_id_t;

#define BspAdcIdFuel       BoardAdcIdFuel
#define BspAdcIdAdPower    BoardAdcIdPower
#define BspAdcIdWaterTemp  BoardAdcIdWaterTemp
#define BspAdcIdIgn        BoardAdcIdIgn
#define BspAdcIdZmIn       BoardAdcIdBrightness
#define BspAdcIdLeftTurn   BoardAdcIdLeftTurn
#define BspAdcIdHighBeam   BoardAdcIdHighBeam
#define BspAdcIdRightTurn  BoardAdcIdRightTurn
#define BspAdcIdCount      BoardAdcIdCount

	void BSP_ADC_Init(void);
	void BSP_ADC_Start(void);
	void BSP_ADC_Stop(void);
	uint16_t BSP_ADC_GetResult(en_bsp_adc_id_t id);
	void BSP_ADC_DeInit(void);
	void BSP_ADC_Wakeup(void);
	void BSP_ADC_WakeupIgnCheck(void);

#ifdef __cplusplus
}
#endif

#endif
