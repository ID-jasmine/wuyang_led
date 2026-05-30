#include "drv_adc.h"

#include "bsp_sys.h"

#define DRV_ADC_SAMPLE_COUNT (10u)
#define DRV_ADC_WAIT_100US	 (1u)
#define DRV_ADC_REF_MV		 (5000u)
#define DRV_ADC_FULL_SCALE	 (4095u)
#define DRV_ADC_IGN_ON_MV	 (1430u)
#define DRV_ADC_IGN_ON_RAW	 (((uint32_t)DRV_ADC_IGN_ON_MV * DRV_ADC_FULL_SCALE + \
							  (DRV_ADC_REF_MV - 1u)) / \
							 DRV_ADC_REF_MV)
#define DRV_ADC_IGN_ON_MS	 (300u)

static uint16_t s_au16DrvAdcAvg[BspAdcIdCount];
static uint32_t s_au32DrvAdcSum[BspAdcIdCount];
static uint8_t s_u8DrvAdcSampleCount = 0u;
static boolean_t s_bDrvAdcReady = FALSE;
static boolean_t s_bDrvAdcInited = FALSE;
static uint16_t s_u16DrvAdcIgnOnCnt = 0u;
static boolean_t s_bDrvAdcIgnActive = FALSE;

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

static void DrvAdc_UpdateIgnState(uint16_t raw)
{
	if (raw >= DRV_ADC_IGN_ON_RAW)
	{
		if (s_u16DrvAdcIgnOnCnt < DRV_ADC_IGN_ON_MS)
		{
			s_u16DrvAdcIgnOnCnt++;
		}

		if (s_u16DrvAdcIgnOnCnt >= DRV_ADC_IGN_ON_MS)
		{
			s_bDrvAdcIgnActive = TRUE;
		}
	}
	else
	{
		s_u16DrvAdcIgnOnCnt = 0u;
		s_bDrvAdcIgnActive = FALSE;
	}
}

/**
 * @brief 初始化 ADC 驱动。
 *
 * 清空采样缓存和平均值状态，并初始化底层 BSP ADC。
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
	s_bDrvAdcInited = FALSE;
	s_u16DrvAdcIgnOnCnt = 0u;
	s_bDrvAdcIgnActive = FALSE;

	BSP_ADC_Init();
	s_bDrvAdcInited = TRUE;
}

/**
 * @brief 执行 ADC 1ms 周期任务。
 *
 * 进行一次采样累加，并在达到设定样本数后更新平均值。
 */
void DRV_ADC_Task1ms(void)
{
	uint8_t id;

	if (FALSE == s_bDrvAdcInited)
	{
		return;
	}

	DrvAdc_StartAndWait();

	for (id = 0u; id < BspAdcIdCount; id++)
	{
		s_au32DrvAdcSum[id] += BSP_ADC_GetResult((en_bsp_adc_id_t)id);
	}

	DrvAdc_UpdateIgnState(BSP_ADC_GetResult(BspAdcIdIgn));

	if (s_u8DrvAdcSampleCount < DRV_ADC_SAMPLE_COUNT)
	{
		s_u8DrvAdcSampleCount++;
	}

	if (s_u8DrvAdcSampleCount >= DRV_ADC_SAMPLE_COUNT)
	{
		for (id = 0u; id < BspAdcIdCount; id++)
		{
			s_au16DrvAdcAvg[id] = (uint16_t)(s_au32DrvAdcSum[id] / DRV_ADC_SAMPLE_COUNT);
			s_au32DrvAdcSum[id] = 0u;
		}
		s_u8DrvAdcSampleCount = 0u;
		s_bDrvAdcReady = TRUE;
	}
}

void DRV_ADC_Task10ms(void)
{
	DRV_ADC_Task1ms();
}

/**
 * @brief 查询 ADC 平均值是否已经准备完成。
 * @return boolean_t `TRUE` 表示至少完成过一轮平均值计算，`FALSE` 表示未准备好。
 */
boolean_t DRV_ADC_IsReady(void)
{
	return s_bDrvAdcReady;
}

boolean_t DRV_ADC_IsIgnActive(void)
{
	return s_bDrvAdcIgnActive;
}

/**
 * @brief 获取指定 ADC 通道的平均值。
 * @param id ADC 通道 ID。
 * @return uint16_t 平均滤波后的采样值；参数非法时返回 `0`。
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
 * @brief 反初始化 ADC 驱动。
 *
 * 停止并关闭底层 ADC 外设。
 */
void DRV_ADC_DeInit(void)
{
	s_bDrvAdcInited = FALSE;
	s_u16DrvAdcIgnOnCnt = 0u;
	s_bDrvAdcIgnActive = FALSE;
	BSP_ADC_DeInit();
}

/**
 * @brief 唤醒 ADC 驱动。
 *
 * 用于从低功耗状态恢复底层 ADC 外设。
 */
void DRV_ADC_Wakeup(void)
{
	BSP_ADC_Wakeup();
	s_bDrvAdcInited = TRUE;
}
