#include "bsp_tm3100_led.h"
#include "gpio.h"

#define TM3100_SDI_PORT GpioPortA
#define TM3100_SDI_PIN	GpioPin8
#define TM3100_CLK_PORT GpioPortB
#define TM3100_CLK_PIN	GpioPin15
#define TM3100_LE_PORT	GpioPortB
#define TM3100_LE_PIN	GpioPin14
#define TM3100_OE_PORT	GpioPortB
#define TM3100_OE_PIN	GpioPin13

static uint16_t s_au16Tm3100Buffer[BSP_TM3100_CHIP_COUNT];
static boolean_t s_bTm3100Inited = FALSE;

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

static void Tm3100_SetOe(boolean_t level)
{
	Tm3100_SetPin(TM3100_OE_PORT, TM3100_OE_PIN, level);
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
	.set_oe = Tm3100_SetOe,
	.delay = Tm3100_Delay,
};

stc_bsp_tm3100_t g_stcTm3100Led = {
	.ops = &s_stcTm3100GpioOps,
	.buffer = s_au16Tm3100Buffer,
	.chip_count = BSP_TM3100_CHIP_COUNT,
	.bit_order = BspTm3100BitMsbFirst,
	.chip_order = BspTm3100ChipLastFirst,
	.oe_active_level = FALSE,
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

static void Tm3100_OutputEnableRaw(boolean_t enable)
{
	boolean_t level;

	level = (enable) ? g_stcTm3100Led.oe_active_level
					 : (boolean_t)!g_stcTm3100Led.oe_active_level;
	g_stcTm3100Led.ops->set_oe(level);
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
	g_stcTm3100Led.ops->set_le(FALSE);
	g_stcTm3100Led.ops->delay();
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

	enRet = Tm3100_InitOutputPin(TM3100_OE_PORT, TM3100_OE_PIN);
	if (Ok != enRet)
	{
		return enRet;
	}

	s_bTm3100Inited = TRUE;
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

	Tm3100_OutputEnableRaw(FALSE);

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
	Tm3100_OutputEnableRaw(TRUE);

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

	Tm3100_OutputEnableRaw(enable);

	return Ok;
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
