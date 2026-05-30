#include "bsp_tm3100_led.h"

#include "bt.h"
#include "gpio.h"
#include "sysctrl.h"

#define TM3100_OE_ON_BRIGHTNESS_PERCENT	 (10u)
#define TM3100_OE_MAX_BRIGHTNESS_PERCENT (100u)
#define TM3100_SDI_PORT					 GpioPortA
#define TM3100_SDI_PIN					 GpioPin8
#define TM3100_CLK_PORT					 GpioPortB
#define TM3100_CLK_PIN					 GpioPin15
#define TM3100_LE_PORT					 GpioPortB
#define TM3100_LE_PIN					 GpioPin14
#define TM3100_OE_TIM_UNIT				 TIM1
#define TM3100_OE_PORT					 GpioPortB
#define TM3100_OE_PIN					 GpioPin13
#define TM3100_OE_AF					 GpioAf5
#define TM3100_OE_PERIOD_TICKS			 (1000u)

static uint16_t s_au16Tm3100Buffer[BSP_TM3100_CHIP_COUNT];
static boolean_t s_bTm3100Inited = FALSE;
static boolean_t s_bTm3100OutputEnabled = FALSE;
static uint8_t s_u8Tm3100OeBrightnessPercent = TM3100_OE_ON_BRIGHTNESS_PERCENT;

static void Tm3100_SetPin(en_gpio_port_t port, en_gpio_pin_t pin, boolean_t level)
{
	if (level)
	{
		(void)Gpio_SetIO(port, pin);
	}
	else
	{
		(void)Gpio_ClrIO(port, pin);
	}
}

static void Tm3100_SetSdi(boolean_t level)
{
	Tm3100_SetPin(TM3100_SDI_PORT, TM3100_SDI_PIN, level);
}

static void Tm3100_SetClk(boolean_t level)
{
	Tm3100_SetPin(TM3100_CLK_PORT, TM3100_CLK_PIN, level);
}

static void Tm3100_SetLe(boolean_t level)
{
	Tm3100_SetPin(TM3100_LE_PORT, TM3100_LE_PIN, level);
}

static void Tm3100_Delay(void)
{
	volatile uint8_t i;

	for (i = 0u; i < 8u; i++)
	{
		__NOP();
	}
}

static const stc_bsp_tm3100_ops_t s_stcTm3100GpioOps = {
	.set_sdi = Tm3100_SetSdi,
	.set_clk = Tm3100_SetClk,
	.set_le = Tm3100_SetLe,
	.delay = Tm3100_Delay,
};

stc_bsp_tm3100_t g_stcTm3100Led = {
	.ops = &s_stcTm3100GpioOps,
	.buffer = s_au16Tm3100Buffer,
	.chip_count = BSP_TM3100_CHIP_COUNT,
	.bit_order = BspTm3100BitMsbFirst,
	.chip_order = BspTm3100ChipLastFirst,
	.reserved = NULL,
};

static en_result_t Tm3100_InitOutputPin(en_gpio_port_t port, en_gpio_pin_t pin)
{
	stc_gpio_cfg_t stcGpioCfg;

	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdDisable;
	stcGpioCfg.enCtrlMode = GpioFastIO;

	return Gpio_Init(port, pin, &stcGpioCfg);
}

static en_result_t Tm3100_CheckReady(void)
{
	if ((FALSE == s_bTm3100Inited) || (NULL == g_stcTm3100Led.ops) ||
		(NULL == g_stcTm3100Led.buffer))
	{
		return ErrorUninitialized;
	}

	return Ok;
}

static uint8_t Tm3100_ClampBrightness(uint8_t brightness_percent)
{
	if (brightness_percent > TM3100_OE_MAX_BRIGHTNESS_PERCENT)
	{
		brightness_percent = TM3100_OE_MAX_BRIGHTNESS_PERCENT;
	}

	return brightness_percent;
}

static uint16_t Tm3100_OePercentToCompare(uint8_t brightness_percent)
{
	uint32_t compare;

	brightness_percent = Tm3100_ClampBrightness(brightness_percent);
	compare = ((uint32_t)TM3100_OE_PERIOD_TICKS * brightness_percent) /
			  TM3100_OE_MAX_BRIGHTNESS_PERCENT;

	if (compare > TM3100_OE_PERIOD_TICKS)
	{
		compare = TM3100_OE_PERIOD_TICKS;
	}

	return (uint16_t)compare;
}

static en_result_t Tm3100_InitOePin(void)
{
	stc_gpio_cfg_t stcGpioCfg;
	en_result_t enRet;

	DDL_ZERO_STRUCT(stcGpioCfg);
	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdDisable;
	stcGpioCfg.enCtrlMode = GpioFastIO;

	enRet = Gpio_Init(TM3100_OE_PORT, TM3100_OE_PIN, &stcGpioCfg);
	if (Ok != enRet)
	{
		return enRet;
	}

	return Gpio_SetAfMode(TM3100_OE_PORT, TM3100_OE_PIN, TM3100_OE_AF);
}

static en_result_t Tm3100_InitOeTimer(void)
{
	stc_bt_mode23_cfg_t stcBaseCfg;
	stc_bt_m23_compare_cfg_t stcCompareCfg;
	en_result_t enRet;

	DDL_ZERO_STRUCT(stcBaseCfg);
	DDL_ZERO_STRUCT(stcCompareCfg);
	Sysctrl_SetPeripheralGate(SysctrlPeripheralBaseTim, TRUE);

	stcBaseCfg.enWorkMode = BtWorkMode2;
	stcBaseCfg.enCT = BtTimer;
	stcBaseCfg.enPRS = BtPCLKDiv16; // 16MHz / 16 = 1MHz, ARR 1000 => 1kHz PWM
	stcBaseCfg.enCntDir = BtCntUp;
	stcBaseCfg.enPWMTypeSel = BtIndependentPWM;
	stcBaseCfg.enPWM2sSel = BtSinglePointCmp;

	enRet = Bt_Mode23_Init(TM3100_OE_TIM_UNIT, &stcBaseCfg);
	if (Ok != enRet)
	{
		return enRet;
	}

	stcCompareCfg.enCh0ACmpCap = BtCHxCmpMode;
	stcCompareCfg.enCH0ACmpCtrl = BtPWMMode1;
	stcCompareCfg.enCH0APolarity = BtPortOpposite;
	stcCompareCfg.bCh0ACmpBufEn = TRUE;
	stcCompareCfg.enCh0ACmpIntSel = BtCmpIntNone;

	stcCompareCfg.enCh0BCmpCap = BtCHxCmpMode;
	stcCompareCfg.enCH0BCmpCtrl = BtForceHigh;
	stcCompareCfg.enCH0BPolarity = BtPortPositive;
	stcCompareCfg.bCH0BCmpBufEn = TRUE;
	stcCompareCfg.enCH0BCmpIntSel = BtCmpIntNone;

	enRet = Bt_M23_PortOutput_Cfg(TM3100_OE_TIM_UNIT, &stcCompareCfg);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Bt_M23_ARRSet(TM3100_OE_TIM_UNIT, TM3100_OE_PERIOD_TICKS, TRUE);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Bt_M23_Cnt16Set(TM3100_OE_TIM_UNIT, 0u);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Bt_ClearAllIntFlag(TM3100_OE_TIM_UNIT);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Bt_M23_EnPWM_Output(TM3100_OE_TIM_UNIT, TRUE, TRUE);
	if (Ok != enRet)
	{
		return enRet;
	}

	return Bt_M23_Run(TM3100_OE_TIM_UNIT);
}

static en_result_t Tm3100_InitOePwm(void)
{
	en_result_t enRet;

	enRet = Tm3100_InitOePin();
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Tm3100_InitOeTimer();
	if (Ok != enRet)
	{
		return enRet;
	}

	return Bt_M23_CCR_Set(TM3100_OE_TIM_UNIT, BtCCR0A, Tm3100_OePercentToCompare(0u));
}

static en_result_t Tm3100_OutputEnableRaw(boolean_t enable)
{
	uint8_t brightness_percent;

	brightness_percent = enable ? s_u8Tm3100OeBrightnessPercent : 0u;

	return Bt_M23_CCR_Set(TM3100_OE_TIM_UNIT, BtCCR0A,
						  Tm3100_OePercentToCompare(brightness_percent));
}

static en_result_t Tm3100_SetOeBrightness(uint8_t brightness_percent)
{
	s_u8Tm3100OeBrightnessPercent = Tm3100_ClampBrightness(brightness_percent);

	if (FALSE == s_bTm3100OutputEnabled)
	{
		return Ok;
	}

	return Tm3100_OutputEnableRaw(TRUE);
}

static void Tm3100_SendBit(boolean_t bit)
{
	g_stcTm3100Led.ops->set_sdi(bit);
	g_stcTm3100Led.ops->delay();

	g_stcTm3100Led.ops->set_clk(TRUE);
	g_stcTm3100Led.ops->delay();

	g_stcTm3100Led.ops->set_clk(FALSE);
	g_stcTm3100Led.ops->delay();
}

static void Tm3100_SendWord(uint16_t data)
{
	int8_t bit;

	if (BspTm3100BitMsbFirst == g_stcTm3100Led.bit_order)
	{
		for (bit = 15; bit >= 0; bit--)
		{
			Tm3100_SendBit((boolean_t)((data >> bit) & 0x0001u));
		}
	}
	else
	{
		for (bit = 0; bit < 16; bit++)
		{
			Tm3100_SendBit((boolean_t)((data >> bit) & 0x0001u));
		}
	}
}

static void Tm3100_Latch(void)
{
	g_stcTm3100Led.ops->set_le(TRUE);
	g_stcTm3100Led.ops->delay();
	g_stcTm3100Led.ops->set_le(FALSE);
}

/**
 * @brief  初始化TM3100 LED驱动模块
 * @return en_result_t 初始化结果
 * @retval Ok: 成功
 * @retval Error: 失败
 */
en_result_t Bsp_Tm3100Led_Init(void)
{
	en_result_t enRet;

	enRet = Tm3100_InitOutputPin(TM3100_SDI_PORT, TM3100_SDI_PIN);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Tm3100_InitOutputPin(TM3100_CLK_PORT, TM3100_CLK_PIN);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Tm3100_InitOutputPin(TM3100_LE_PORT, TM3100_LE_PIN);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = Tm3100_InitOePwm();
	if (Ok != enRet)
	{
		return enRet;
	}

	s_bTm3100Inited = TRUE;
	s_bTm3100OutputEnabled = FALSE;
	s_u8Tm3100OeBrightnessPercent = TM3100_OE_ON_BRIGHTNESS_PERCENT;
	(void)Bsp_Tm3100Led_OutputEnable(FALSE);
	g_stcTm3100Led.ops->set_sdi(FALSE);
	g_stcTm3100Led.ops->set_clk(FALSE);
	g_stcTm3100Led.ops->set_le(FALSE);
	(void)Bsp_Tm3100Led_Clear();
	(void)Bsp_Tm3100Led_Refresh();

	return Ok;
}

/**
 * @brief  设置指定的TM3100芯片及其通道状态
 * @param  [in] chip 芯片索引，范围：0 ~ (BSP_TM3100_CHIP_COUNT - 1)
 * @param  [in] channel 通道索引，范围：0 ~ (BSP_TM3100_CHANNEL_PER_CHIP - 1)
 * @param  [in] on TRUE: 点亮 (置1) / FALSE: 熄灭 (清0)
 * @return en_result_t 设置结果
 * @retval Ok: 成功
 * @retval ErrorInvalidParameter: 芯片或通道索引越界
 */
en_result_t Bsp_Tm3100Led_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t on)
{
	if ((chip >= g_stcTm3100Led.chip_count) || (channel >= BSP_TM3100_CHANNEL_PER_CHIP))
	{
		return ErrorInvalidParameter;
	}

	if (on)
	{
		g_stcTm3100Led.buffer[chip] |= (uint16_t)(1u << channel);
	}
	else
	{
		g_stcTm3100Led.buffer[chip] &= (uint16_t)~(1u << channel);
	}

	return Ok;
}

/**
 * @brief  以线性索引方式设置LED通道状态
 * @param  [in] index LED的线性索引位置，范围：0 ~ (BSP_TM3100_LED_COUNT - 1)
 * @param  [in] on TRUE: 点亮 (置1) / FALSE: 熄灭 (清0)
 * @return en_result_t 设置结果
 */
en_result_t Bsp_Tm3100Led_SetLinear(uint16_t index, boolean_t on)
{
	uint8_t chip;
	uint8_t channel;

	if (index >= BSP_TM3100_LED_COUNT)
	{
		return ErrorInvalidParameter;
	}

	chip = (uint8_t)(index / BSP_TM3100_CHANNEL_PER_CHIP);
	channel = (uint8_t)(index % BSP_TM3100_CHANNEL_PER_CHIP);

	return Bsp_Tm3100Led_SetChipChannel(chip, channel, on);
}

/**
 * @brief  直接设置整个TM3100芯片的16位数据
 * @param  [in] chip 芯片索引，范围：0 ~ (BSP_TM3100_CHIP_COUNT - 1)
 * @param  [in] data 要写入该芯片的16位数据
 * @return en_result_t 设置结果
 */
en_result_t Bsp_Tm3100Led_SetChipData(uint8_t chip, uint16_t data)
{
	if (chip >= g_stcTm3100Led.chip_count)
	{
		return ErrorInvalidParameter;
	}

	g_stcTm3100Led.buffer[chip] = data;

	return Ok;
}

/**
 * @brief  清空(熄灭)所有TM3100芯片的通道数据(仅更新缓存，需调用Refresh生效)
 * @return en_result_t 清空结果
 */
en_result_t Bsp_Tm3100Led_Clear(void)
{
	uint8_t chip;

	for (chip = 0u; chip < g_stcTm3100Led.chip_count; chip++)
	{
		g_stcTm3100Led.buffer[chip] = 0x0000u;
	}

	return Ok;
}

/**
 * @brief  填充(点亮)所有TM3100芯片的通道数据(仅更新缓存，需调用Refresh生效)
 * @return en_result_t 填充结果
 */
en_result_t Bsp_Tm3100Led_Fill(void)
{
	uint8_t chip;

	for (chip = 0u; chip < g_stcTm3100Led.chip_count; chip++)
	{
		g_stcTm3100Led.buffer[chip] = 0xFFFFu;
	}

	return Ok;
}

/**
 * @brief  将缓存中的数据刷新并发送至硬件，锁存显示
 * @return en_result_t 刷新结果
 */
en_result_t Bsp_Tm3100Led_Refresh(void)
{
	int8_t chip;
	en_result_t enRet;

	enRet = Tm3100_CheckReady();
	if (Ok != enRet)
	{
		return enRet;
	}

	g_stcTm3100Led.ops->set_le(FALSE);

	if (BspTm3100ChipLastFirst == g_stcTm3100Led.chip_order)
	{
		for (chip = (int8_t)(g_stcTm3100Led.chip_count - 1u); chip >= 0; chip--)
		{
			Tm3100_SendWord(g_stcTm3100Led.buffer[(uint8_t)chip]);
		}
	}
	else
	{
		for (chip = 0; chip < (int8_t)g_stcTm3100Led.chip_count; chip++)
		{
			Tm3100_SendWord(g_stcTm3100Led.buffer[(uint8_t)chip]);
		}
	}

	Tm3100_Latch();

	return Ok;
}

/**
 * @brief  控制TM3100驱动输出使能/关闭
 * @param  [in] enable TRUE: 输出使能 / FALSE: 输出关闭
 * @return en_result_t 控制结果
 */
en_result_t Bsp_Tm3100Led_OutputEnable(boolean_t enable)
{
	en_result_t enRet;

	enRet = Tm3100_CheckReady();
	if (Ok != enRet)
	{
		return enRet;
	}

	s_bTm3100OutputEnabled = enable;

	return Tm3100_OutputEnableRaw(enable);
}

/**
 * @brief  设置TM3100 OE PWM占空比
 * @param  [in] brightness_percent 亮度百分比，范围0 ~ 100，超过100时按100处理
 * @return en_result_t 设置结果
 */
en_result_t Bsp_Tm3100Led_SetBrightness(uint8_t brightness_percent)
{
	en_result_t enRet;

	enRet = Tm3100_CheckReady();
	if (Ok != enRet)
	{
		return enRet;
	}

	return Tm3100_SetOeBrightness(brightness_percent);
}

/**
 * @brief  获取TM3100 OE PWM占空比
 * @return uint8_t 当前亮度百分比
 */
uint8_t Bsp_Tm3100Led_GetBrightness(void)
{
	return s_u8Tm3100OeBrightnessPercent;
}

/**
 * @brief  获取指定TM3100芯片当前的通道数据(缓存值)
 * @param  [in] chip 芯片索引
 * @return uint16_t 对应的16位数据，越界则返回0
 */
uint16_t Bsp_Tm3100Led_GetChipData(uint8_t chip)
{
	if (chip >= g_stcTm3100Led.chip_count)
	{
		return 0u;
	}

	return g_stcTm3100Led.buffer[chip];
}

/**
 * @brief  获取TM3100所有的芯片缓存数据指针
 * @return uint16_t* 缓存数组指针
 */
uint16_t *Bsp_Tm3100Led_GetBuffer(void)
{
	return g_stcTm3100Led.buffer;
}
