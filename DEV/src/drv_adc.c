#include "drv_adc.h"

#include "bsp_sys.h"

#define DRV_ADC_SAMPLE_COUNT (10u)
#define DRV_ADC_WAIT_100US	 (1u)

static uint16_t s_au16DrvAdcAvg[BspAdcIdCount];
static uint32_t s_au32DrvAdcSum[BspAdcIdCount];
static uint8_t s_u8DrvAdcSampleCount = 0u;
static boolean_t s_bDrvAdcReady = FALSE;

static en_result_t DrvAdc_CheckId(en_bsp_adc_id_t id)
{
	if (id >= BspAdcIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static void DrvAdc_StartAndWait(void)
{
	BSP_ADC_Start();
	delay100us_safe(DRV_ADC_WAIT_100US);
}

/**
 * @brief  ADC设备抽象层初始化（清空缓存均值，初始化底层BSP）
 */
void DRV_ADC_Init(void)
{
	uint8_t i;

	for (i = 0u; i < BspAdcIdCount; i++)
	{
		s_au16DrvAdcAvg[i] = 0u;
		s_au32DrvAdcSum[i] = 0u;
	}
	s_u8DrvAdcSampleCount = 0u;
	s_bDrvAdcReady = FALSE;

	BSP_ADC_Init();
}

/**
 * @brief  执行ADC数据更新任务（连续多次采样并计算平均值，实现软件滤波）
 */
void DRV_ADC_Task10ms(void)
{
	uint8_t id;

	DrvAdc_StartAndWait();

	for (id = 0u; id < BspAdcIdCount; id++)
	{
		s_au32DrvAdcSum[id] += BSP_ADC_GetResult((en_bsp_adc_id_t)id);
	}

	if (s_u8DrvAdcSampleCount < DRV_ADC_SAMPLE_COUNT)
	{
		s_u8DrvAdcSampleCount++;
	}

	if (s_u8DrvAdcSampleCount >= DRV_ADC_SAMPLE_COUNT)
	{
		for (id = 0u; id < BspAdcIdCount; id++)
		{
			s_au16DrvAdcAvg[id] =
				(uint16_t)(s_au32DrvAdcSum[id] / DRV_ADC_SAMPLE_COUNT);
			s_au32DrvAdcSum[id] = 0u;
		}
		s_u8DrvAdcSampleCount = 0u;
		s_bDrvAdcReady = TRUE;
	}
}

boolean_t DRV_ADC_IsReady(void)
{
	return s_bDrvAdcReady;
}

/**
 * @brief  获取指定ADC通道经过平均滤波后的采样值
 * @param  id ADC通道逻辑ID (例如: 燃油、电源电压、水温)
 * @return uint16_t 平均滤波后的ADC数值
 */
uint16_t DRV_ADC_GetAvg(en_bsp_adc_id_t id)
{
	if (Ok != DrvAdc_CheckId(id))
	{
		return 0u;
	}

	return s_au16DrvAdcAvg[id];
}

/**
 * @brief  ADC设备抽象层反初始化（停止并关闭底层ADC以节省功耗）
 */
void DRV_ADC_DeInit(void)
{
	BSP_ADC_DeInit();
}

/**
 * @brief  ADC设备抽象层唤醒（从低功耗状态中恢复底层ADC）
 */
void DRV_ADC_Wakeup(void)
{
	BSP_ADC_Wakeup();
}
