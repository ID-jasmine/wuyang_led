#include "bsp_adc.h"

#include "adc.h"
#include "bsp_sys.h"
#include "gpio.h"

static uint8_t s_au8BspAdcSqrIndex[BspAdcIdCount];

static en_adc_sqr_chmux_t BspAdc_GetSqrMux(uint8_t index)
{
	return (en_adc_sqr_chmux_t)index;
}

static en_result_t BspAdc_CheckId(en_bsp_adc_id_t id)
{
	if ((id >= BspAdcIdCount) || (FALSE == g_astBoardAdcCfg[id].enabled))
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static void BspAdc_PortInit(void)
{
	uint8_t i;
	const stc_board_pin_cfg_t *pstcPinCfg;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	for (i = 0u; i < BspAdcIdCount; i++)
	{
		if (FALSE == g_astBoardAdcCfg[i].enabled)
		{
			continue;
		}
		pstcPinCfg = Board_GetPinConfig(g_astBoardAdcCfg[i].pin_id);
		if (NULL != pstcPinCfg)
		{
			Gpio_SetAnalogMode(pstcPinCfg->port, pstcPinCfg->pin);
		}
	}
}

static void BspAdc_PortInitById(en_bsp_adc_id_t id)
{
	const stc_board_pin_cfg_t *pstcPinCfg;

	if (Ok != BspAdc_CheckId(id))
	{
		return;
	}

	pstcPinCfg = Board_GetPinConfig(g_astBoardAdcCfg[id].pin_id);
	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
	Gpio_SetAnalogMode(pstcPinCfg->port, pstcPinCfg->pin);
}

static void BspAdc_CoreInit(void)
{
	stc_adc_sqr_cfg_t stcAdcSqrCfg;
	uint8_t i;
	uint8_t active_count = 0u;

	DDL_ZERO_STRUCT(stcAdcSqrCfg);

	Sysctrl_SetPeripheralGate(SysctrlPeripheralAdcBgr, TRUE);

	M0P_BGR->CR |= 0x1u;
	delay100us_safe(1);

	M0P_ADC->CR0 = 0x1u;
	delay100us_safe(1);

	M0P_ADC->CR0 |= (uint32_t)AdcMskClkDiv1 | (uint32_t)AdcMskRefVolSelAVDD |
					(uint32_t)AdcMskBufDisable | (uint32_t)AdcMskSampCycle8Clk |
					(uint32_t)AdcMskInRefDisable;

	M0P_ADC->CR1_f.MODE = AdcScanMode;
	M0P_ADC->CR1_f.ALIGN = AdcAlignRight;

	for (i = 0u; i < BspAdcIdCount; i++)
	{
		s_au8BspAdcSqrIndex[i] = 0xFFu;
		if (TRUE == g_astBoardAdcCfg[i].enabled)
		{
			s_au8BspAdcSqrIndex[i] = active_count;
			active_count++;
		}
	}

	stcAdcSqrCfg.u8SqrCnt = active_count;
	stcAdcSqrCfg.enResultAcc = AdcResultAccDisable;
	stcAdcSqrCfg.bSqrDmaTrig = FALSE;
	Adc_SqrModeCfg(&stcAdcSqrCfg);

	for (i = 0u; i < BspAdcIdCount; i++)
	{
		if (TRUE == g_astBoardAdcCfg[i].enabled)
		{
			Adc_CfgSqrChannel(BspAdc_GetSqrMux(s_au8BspAdcSqrIndex[i]),
							  g_astBoardAdcCfg[i].channel);
		}
	}

	Adc_Enable();
}

/**
 * @brief  ADC模块初始化（包含GPIO端口初始化与ADC核心配置）
 */
void BSP_ADC_Init(void)
{
	BspAdc_PortInit();
	BspAdc_CoreInit();
}

/**
 * @brief  ADC唤醒（从休眠/低功耗模式恢复，重新开启基准电压和ADC测压模块）
 */
void BSP_ADC_Wakeup(void)
{
	BspAdc_PortInit();

	M0P_BGR->CR |= 0x1u;
	delay100us_safe(1);

	M0P_ADC->CR0 |= 0x1u;
	delay100us_safe(1);

	Adc_Enable();
}

/**
 * @brief  仅恢复IGN采样所需的ADC资源，用于低功耗唤醒后的电门确认。
 */
void BSP_ADC_WakeupIgnCheck(void)
{
	BspAdc_PortInitById(BspAdcIdIgn);

	M0P_BGR->CR |= 0x1u;
	delay100us_safe(1);

	M0P_ADC->CR0 |= 0x1u;
	delay100us_safe(1);

	Adc_Enable();
}

/**
 * @brief  启动ADC扫描（Sequence）转换
 */
void BSP_ADC_Start(void)
{
	Adc_SQR_Start();
}

/**
 * @brief  停止ADC扫描转换
 */
void BSP_ADC_Stop(void)
{
	Adc_SQR_Stop();
}

/**
 * @brief  获取指定ADC通道的转换结果
 * @param  id ADC通道逻辑ID (例如: 燃油、电源电压、水温)
 * @return uint16_t 对应通道的12位ADC原始采样值
 */
uint16_t BSP_ADC_GetResult(en_bsp_adc_id_t id)
{
	if (Ok != BspAdc_CheckId(id))
	{
		return 0u;
	}

	return (uint16_t)Adc_GetSqrResult(BspAdc_GetSqrMux(s_au8BspAdcSqrIndex[id]));
}

/**
 * @brief  ADC模块反初始化（关闭ADC，关闭相关电源模块以降低功耗）
 */
void BSP_ADC_DeInit(void)
{
	BSP_ADC_Stop();
	Adc_Disable();
	M0P_ADC->CR0 &= ~0x1u;
	M0P_BGR->CR &= ~0x1u;
}
