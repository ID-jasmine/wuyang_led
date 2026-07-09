#include "bsp_adc.h"

#include "adc.h"
#include "bsp_sys.h"
#include "gpio.h"

typedef struct stc_bsp_adc_channel_cfg
{
	en_gpio_port_t port;
	en_gpio_pin_t pin;
	en_adc_samp_ch_sel_t adc_ch;
} stc_bsp_adc_channel_cfg_t;

static const stc_bsp_adc_channel_cfg_t s_astBspAdcChannelCfg[BspAdcIdCount] = {
	[BspAdcIdFuel] =
		{
			.port = GpioPortA,
			.pin = GpioPin1,
			.adc_ch = AdcExInputCH1,
		},
	[BspAdcIdAdPower] =
		{
			.port = GpioPortA,
			.pin = GpioPin0,
			.adc_ch = AdcExInputCH0,
		},
	[BspAdcIdWaterTemp] =
		{
			.port = GpioPortA,
			.pin = GpioPin2,
			.adc_ch = AdcExInputCH2,
		},
	[BspAdcIdIgn] =
		{
			.port = GpioPortB,
			.pin = GpioPin11,
			.adc_ch = AdcExInputCH18,
		},
	[BspAdcIdZmIn] =
		{
			.port = GpioPortB,
			.pin = GpioPin12,
			.adc_ch = AdcExInputCH19,
		},
	[BspAdcIdLeftTurn] =
		{
			.port = GpioPortA,
			.pin = GpioPin5,
			.adc_ch = AdcExInputCH5,
		},
	[BspAdcIdHighBeam] =
		{
			.port = GpioPortA,
			.pin = GpioPin6,
			.adc_ch = AdcExInputCH6,
		},
	[BspAdcIdRightTurn] =
		{
			.port = GpioPortA,
			.pin = GpioPin4,
			.adc_ch = AdcExInputCH4,
		},
};

static en_adc_sqr_chmux_t BspAdc_GetSqrMux(uint8_t index)
{
	return (en_adc_sqr_chmux_t)index;
}

static en_result_t BspAdc_CheckId(en_bsp_adc_id_t id)
{
	if (id >= BspAdcIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static void BspAdc_PortInit(void)
{
	uint8_t i;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	for (i = 0u; i < BspAdcIdCount; i++)
	{
		Gpio_SetAnalogMode(s_astBspAdcChannelCfg[i].port, s_astBspAdcChannelCfg[i].pin);
	}
}

static void BspAdc_PortInitById(en_bsp_adc_id_t id)
{
	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
	Gpio_SetAnalogMode(s_astBspAdcChannelCfg[id].port, s_astBspAdcChannelCfg[id].pin);
}

static void BspAdc_CoreInit(void)
{
	stc_adc_sqr_cfg_t stcAdcSqrCfg;
	uint8_t i;

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

	stcAdcSqrCfg.u8SqrCnt = BspAdcIdCount;
	stcAdcSqrCfg.enResultAcc = AdcResultAccDisable;
	stcAdcSqrCfg.bSqrDmaTrig = FALSE;
	Adc_SqrModeCfg(&stcAdcSqrCfg);

	for (i = 0u; i < BspAdcIdCount; i++)
	{
		Adc_CfgSqrChannel(BspAdc_GetSqrMux(i), s_astBspAdcChannelCfg[i].adc_ch);
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

	return (uint16_t)Adc_GetSqrResult(BspAdc_GetSqrMux((uint8_t)id));
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
