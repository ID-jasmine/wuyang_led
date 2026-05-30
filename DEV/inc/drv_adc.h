#ifndef __DRV_ADC_H__
#define __DRV_ADC_H__

#include "bsp_adc.h"
#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DRV_ADC_RESISTANCE_INVALID_OHM (0xFFFFu)

void DRV_ADC_Init(void);
void DRV_ADC_Task1ms(void);
void DRV_ADC_Task10ms(void);
boolean_t DRV_ADC_IsReady(void);
boolean_t DRV_ADC_IsIgnActive(void);
uint16_t DRV_ADC_GetAvg(en_bsp_adc_id_t id);
uint16_t DRV_ADC_GetResistanceOhm(en_bsp_adc_id_t id);
void DRV_ADC_DeInit(void);
void DRV_ADC_Wakeup(void);

#ifdef __cplusplus
}
#endif

#endif
