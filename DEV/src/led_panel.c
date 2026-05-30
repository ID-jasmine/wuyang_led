#include "led_panel.h"

#define LED_PANEL_SEG_A (0x01u)
#define LED_PANEL_SEG_B (0x02u)
#define LED_PANEL_SEG_C (0x04u)
#define LED_PANEL_SEG_D (0x08u)
#define LED_PANEL_SEG_E (0x10u)
#define LED_PANEL_SEG_F (0x20u)
#define LED_PANEL_SEG_G (0x40u)

static const uint8_t s_au8LedPanelDigitPattern[10] = {
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_B | LED_PANEL_SEG_C | LED_PANEL_SEG_D |
			  LED_PANEL_SEG_E | LED_PANEL_SEG_F),
	(uint8_t)(LED_PANEL_SEG_B | LED_PANEL_SEG_C),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_B | LED_PANEL_SEG_D | LED_PANEL_SEG_E |
			  LED_PANEL_SEG_G),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_B | LED_PANEL_SEG_C | LED_PANEL_SEG_D |
			  LED_PANEL_SEG_G),
	(uint8_t)(LED_PANEL_SEG_B | LED_PANEL_SEG_C | LED_PANEL_SEG_F | LED_PANEL_SEG_G),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_C | LED_PANEL_SEG_D | LED_PANEL_SEG_F |
			  LED_PANEL_SEG_G),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_C | LED_PANEL_SEG_D | LED_PANEL_SEG_E |
			  LED_PANEL_SEG_F | LED_PANEL_SEG_G),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_B | LED_PANEL_SEG_C),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_B | LED_PANEL_SEG_C | LED_PANEL_SEG_D |
			  LED_PANEL_SEG_E | LED_PANEL_SEG_F | LED_PANEL_SEG_G),
	(uint8_t)(LED_PANEL_SEG_A | LED_PANEL_SEG_B | LED_PANEL_SEG_C | LED_PANEL_SEG_D |
			  LED_PANEL_SEG_F | LED_PANEL_SEG_G),
};

static en_result_t LedPanel_SetImpl(en_led_panel_id_t led_id, boolean_t level)
{
	const stc_led_panel_map_t *pstcMap;
	uint8_t i;
	en_result_t enRet;

	if (led_id >= g_stcLedPanel.count)
	{
		return ErrorInvalidParameter;
	}

	pstcMap = &g_stcLedPanel.map[led_id];
	if ((NULL == pstcMap->points) || (0u == pstcMap->count))
	{
		return ErrorInvalidParameter;
	}

	for (i = 0u; i < pstcMap->count; i++)
	{
		enRet = Bsp_Tm3100Led_SetChipChannel(pstcMap->points[i].chip,
											 pstcMap->points[i].channel, level);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

static en_result_t LedPanel_SetIndexList(const uint8_t *indices, uint8_t count,
										 boolean_t level)
{
	uint8_t i;
	en_result_t enRet;

	if ((NULL == indices) || (0u == count))
	{
		return ErrorInvalidParameter;
	}

	for (i = 0u; i < count; i++)
	{
		enRet = Bsp_Tm3100Led_SetLinear(indices[i], level);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

static en_result_t LedPanel_SetSegmentGroups(const stc_led_panel_index_group_t *segments,
											 uint8_t digit)
{
	uint8_t i;
	uint8_t pattern;
	en_result_t enRet;

	if (NULL == segments)
	{
		return ErrorInvalidParameter;
	}

	for (i = 0u; i < 7u; i++)
	{
		enRet = LedPanel_SetIndexList(segments[i].indices, segments[i].count, FALSE);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	if (digit > 9u)
	{
		return Ok;
	}

	pattern = s_au8LedPanelDigitPattern[digit];
	for (i = 0u; i < 7u; i++)
	{
		if (0u != (pattern & (uint8_t)(1u << i)))
		{
			enRet = LedPanel_SetIndexList(segments[i].indices, segments[i].count, TRUE);
			if (Ok != enRet)
			{
				return enRet;
			}
		}
	}

	return Ok;
}

static en_result_t LedPanel_SetSevenSegment(const uint8_t *segments, uint8_t digit)
{
	uint8_t i;
	uint8_t pattern;
	en_result_t enRet;

	if (NULL == segments)
	{
		return ErrorInvalidParameter;
	}

	for (i = 0u; i < 7u; i++)
	{
		enRet = Bsp_Tm3100Led_SetLinear(segments[i], FALSE);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	if (digit > 9u)
	{
		return Ok;
	}

	pattern = s_au8LedPanelDigitPattern[digit];
	for (i = 0u; i < 7u; i++)
	{
		if (0u != (pattern & (uint8_t)(1u << i)))
		{
			enRet = Bsp_Tm3100Led_SetLinear(segments[i], TRUE);
			if (Ok != enRet)
			{
				return enRet;
			}
		}
	}

	return Ok;
}

static en_result_t LedPanel_SetSpeedHundreds(boolean_t enable)
{
	en_result_t enRet;

	enRet = LedPanel_SetIndexList(g_au8LedPanelSpeedHundredsSegB,
								  ARRAY_SZ(g_au8LedPanelSpeedHundredsSegB), enable);
	if (Ok != enRet)
	{
		return enRet;
	}

	return LedPanel_SetIndexList(g_au8LedPanelSpeedHundredsSegC,
								 ARRAY_SZ(g_au8LedPanelSpeedHundredsSegC), enable);
}

static en_result_t LedPanel_ShowBars(const uint8_t *indices, uint8_t total_count,
									 uint8_t bar_count)
{
	uint8_t i;
	en_result_t enRet;

	if (NULL == indices)
	{
		return ErrorInvalidParameter;
	}

	if (bar_count > total_count)
	{
		bar_count = total_count;
	}

	for (i = 0u; i < total_count; i++)
	{
		enRet = Bsp_Tm3100Led_SetLinear(indices[i], (boolean_t)(i < bar_count));
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

static const stc_led_panel_ops_t s_stcLedPanelOps = {
	.init = Bsp_Tm3100Led_Init,
	.set = LedPanel_SetImpl,
	.set_chip_channel = Bsp_Tm3100Led_SetChipChannel,
	.clear = Bsp_Tm3100Led_Clear,
	.fill = Bsp_Tm3100Led_Fill,
	.refresh = Bsp_Tm3100Led_Refresh,
	.output_enable = Bsp_Tm3100Led_OutputEnable,
	.set_brightness = Bsp_Tm3100Led_SetBrightness,
	.get_brightness = Bsp_Tm3100Led_GetBrightness,
};

stc_led_panel_t g_stcLedPanel = {
	.ops = &s_stcLedPanelOps,
	.map = g_astLedPanelMap,
	.count = (uint16_t)LedPanelIdCount,
};

/**
 * @brief 初始化 LED 面板驱动。
 * @return en_result_t 初始化结果。
 */
en_result_t LedPanel_Init(void)
{
	return g_stcLedPanel.ops->init();
}

/**
 * @brief 按逻辑 LED ID 设置面板灯状态。
 * @param led_id 逻辑 LED ID。
 * @param level 输出电平，`TRUE` 表示点亮，`FALSE` 表示熄灭。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_Set(en_led_panel_id_t led_id, boolean_t level)
{
	return g_stcLedPanel.ops->set(led_id, level);
}

/**
 * @brief 直接设置指定驱动芯片通道的输出状态。
 * @param chip 芯片序号。
 * @param channel 通道序号。
 * @param level 输出电平，`TRUE` 表示点亮，`FALSE` 表示熄灭。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t level)
{
	return g_stcLedPanel.ops->set_chip_channel(chip, channel, level);
}

/**
 * @brief 清空 LED 面板缓存状态。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_Clear(void)
{
	return g_stcLedPanel.ops->clear();
}

/**
 * @brief 将 LED 面板缓存填充为全亮状态。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_Fill(void)
{
	return g_stcLedPanel.ops->fill();
}

/**
 * @brief 刷新 LED 面板缓存到物理驱动芯片。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_Refresh(void)
{
	return g_stcLedPanel.ops->refresh();
}

/**
 * @brief 控制 LED 驱动芯片的全局输出使能。
 * @param enable `TRUE` 表示使能输出，`FALSE` 表示禁止输出。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_OutputEnable(boolean_t enable)
{
	return g_stcLedPanel.ops->output_enable(enable);
}

/**
 * @brief 设置 LED 面板 OE PWM 占空比。
 * @param brightness_percent 亮度百分比，0 ~ 100。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_SetBrightness(uint8_t brightness_percent)
{
	return g_stcLedPanel.ops->set_brightness(brightness_percent);
}

/**
 * @brief 获取 LED 面板 OE PWM 占空比。
 * @return uint8_t 当前亮度百分比。
 */
uint8_t LedPanel_GetBrightness(void)
{
	return g_stcLedPanel.ops->get_brightness();
}

/**
 * @brief 设置 LED 面板边框灯状态。
 * @param level 输出电平，`TRUE` 表示点亮，`FALSE` 表示熄灭。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_SetBorder(boolean_t level)
{
	return LedPanel_Set(LedPanelIdBorder, level);
}

/**
 * @brief 在 LED 面板上显示时钟。
 * @param hour 小时值 (0 ~ 99)。
 * @param minute 分钟值 (0 ~ 99)。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowClock(uint8_t hour, uint8_t minute)
{
	en_result_t enRet;

	if ((hour != 0xFFu && hour > 99u) || (minute != 0xFFu && minute > 99u))
	{
		return ErrorInvalidParameter;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelClockHourTensSegments,
									 (hour == 0xFFu) ? 0xFFu : (uint8_t)(hour / 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelClockHourOnesSegments,
									 (hour == 0xFFu) ? 0xFFu : (uint8_t)(hour % 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelClockMinuteTensSegments,
									 (minute == 0xFFu) ? 0xFFu : (uint8_t)(minute / 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	return LedPanel_SetSevenSegment(g_au8LedPanelClockMinuteOnesSegments,
									(minute == 0xFFu) ? 0xFFu : (uint8_t)(minute % 10u));
}

/**
 * @brief 在 LED 面板上显示档位数字。
 * @param digit 档位数字 (0 ~ 9)。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowGearDigit(uint8_t digit)
{
	if (digit > 9u)
	{
		return ErrorInvalidParameter;
	}

	return LedPanel_SetSevenSegment(g_au8LedPanelGearSegments, digit);
}

en_result_t LedPanel_ClearGearDigit(void)
{
	return LedPanel_SetSevenSegment(g_au8LedPanelGearSegments, 0xFFu);
}

/**
 * @brief 在 LED 面板上显示车速。
 * @param speed 车速值 (0 ~ 199)。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowSpeed(uint16_t speed)
{
	uint8_t tens_digit;
	uint8_t ones_digit;
	en_result_t enRet;

	if (speed > 199u)
	{
		speed = 199u;
	}

	enRet = LedPanel_SetSpeedHundreds((boolean_t)(speed >= 100u));
	if (Ok != enRet)
	{
		return enRet;
	}

	tens_digit = (uint8_t)((speed / 10u) % 10u);
	ones_digit = (uint8_t)(speed % 10u);

	enRet = LedPanel_SetSegmentGroups(g_astLedPanelSpeedTensSegments,
									  (speed >= 10u) ? tens_digit : 0xFFu);
	if (Ok != enRet)
	{
		return enRet;
	}

	return LedPanel_SetSegmentGroups(g_astLedPanelSpeedOnesSegments, ones_digit);
}

/**
 * @brief 在 LED 面板上显示总里程/单次里程。
 * @param value 里程数值 (0 ~ 99999)。
 * @param decimal_digit 小数位数值 (0 ~ 9)。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowOdometer(uint32_t value, uint8_t decimal_digit)
{
	en_result_t enRet;

	if ((value > 99999u) || (decimal_digit > 9u))
	{
		return ErrorInvalidParameter;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoTenThousandsSegments,
									 (uint8_t)((value / 10000u) % 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoThousandsSegments,
									 (uint8_t)((value / 1000u) % 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoHundredsSegments,
									 (uint8_t)((value / 100u) % 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoTensSegments,
									 (uint8_t)((value / 10u) % 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet =
		LedPanel_SetSevenSegment(g_au8LedPanelOdoOnesSegments, (uint8_t)(value % 10u));
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoDecimalSegments, decimal_digit);
	if (Ok != enRet)
	{
		return enRet;
	}

	return Bsp_Tm3100Led_SetLinear(g_u8LedPanelOdoDecimalPointIndex, TRUE);
}

/**
 * @brief 在 LED 面板上按格数显示发动机转速。
 * @param bar_count 点亮的格数索引。
 * @return en_result_t 操作结果。
 */
static en_result_t LedPanel_ShowOdometerDigits(const uint8_t digits[6],
											   boolean_t decimal_point)
{
	en_result_t enRet;

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoTenThousandsSegments, digits[0]);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoThousandsSegments, digits[1]);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoHundredsSegments, digits[2]);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoTensSegments, digits[3]);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoOnesSegments, digits[4]);
	if (Ok != enRet)
	{
		return enRet;
	}

	enRet = LedPanel_SetSevenSegment(g_au8LedPanelOdoDecimalSegments, digits[5]);
	if (Ok != enRet)
	{
		return enRet;
	}

	return Bsp_Tm3100Led_SetLinear(g_u8LedPanelOdoDecimalPointIndex, decimal_point);
}

en_result_t LedPanel_ShowTripOdometer(uint16_t tenths_km)
{
	uint16_t integer_km;
	uint8_t digits[6];

	if (tenths_km > 9999u)
	{
		tenths_km = 9999u;
	}

	integer_km = (uint16_t)(tenths_km / 10u);
	digits[0] = 0xFFu;
	digits[1] = 0xFFu;
	digits[2] = (integer_km >= 100u) ? (uint8_t)(integer_km / 100u) : 0xFFu;
	digits[3] = (integer_km >= 10u) ? (uint8_t)((integer_km / 10u) % 10u) : 0xFFu;
	digits[4] = (uint8_t)(integer_km % 10u);
	digits[5] = (uint8_t)(tenths_km % 10u);

	return LedPanel_ShowOdometerDigits(digits, TRUE);
}

en_result_t LedPanel_ShowTotalOdometer(uint32_t km)
{
	uint8_t digits[6];
	boolean_t significant;
	uint32_t divisor;
	uint8_t i;

	if (km > 999999u)
	{
		km = 999999u;
	}

	significant = FALSE;
	divisor = 100000u;
	for (i = 0u; i < 6u; i++)
	{
		digits[i] = (uint8_t)((km / divisor) % 10u);
		if ((digits[i] != 0u) || (i == 5u))
		{
			significant = TRUE;
		}
		else if (FALSE == significant)
		{
			digits[i] = 0xFFu;
		}
		divisor /= 10u;
	}

	return LedPanel_ShowOdometerDigits(digits, FALSE);
}

en_result_t LedPanel_ShowRpmBars(uint8_t bar_count)
{
	return LedPanel_ShowBars(g_au8LedPanelRpmBarIndices,
							 ARRAY_SZ(g_au8LedPanelRpmBarIndices), bar_count);
}

/**
 * @brief 在 LED 面板上按百分比显示发动机转速。
 * @param percent 转速百分比 (0 ~ 100)。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowRpmPercent(uint8_t percent)
{
	uint8_t bar_count;

	if (percent > 100u)
	{
		percent = 100u;
	}

	bar_count =
		(uint8_t)(((uint16_t)percent * ARRAY_SZ(g_au8LedPanelRpmBarIndices) + 99u) /
				  100u);

	return LedPanel_ShowRpmBars(bar_count);
}

/**
 * @brief 在 LED 面板上按格数显示油量。
 * @param bar_count 点亮的格数。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowFuelBars(uint8_t bar_count)
{
	return LedPanel_ShowBars(g_au8LedPanelFuelBarIndices,
							 ARRAY_SZ(g_au8LedPanelFuelBarIndices), bar_count);
}

/**
 * @brief 在 LED 面板上按百分比显示油量。
 * @param percent 油量百分比 (0 ~ 100)。
 * @return en_result_t 操作结果。
 */
en_result_t LedPanel_ShowFuelPercent(uint8_t percent)
{
	uint8_t bar_count;

	if (percent > 100u)
	{
		percent = 100u;
	}

	bar_count =
		(uint8_t)(((uint16_t)percent * ARRAY_SZ(g_au8LedPanelFuelBarIndices) + 99u) /
				  100u);

	return LedPanel_ShowFuelBars(bar_count);
}
