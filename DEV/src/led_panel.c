#include "led_panel.h"

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

static const stc_led_panel_ops_t s_stcLedPanelOps = {
	.init = Bsp_Tm3100Led_Init,
	.set = LedPanel_SetImpl,
	.set_chip_channel = Bsp_Tm3100Led_SetChipChannel,
	.clear = Bsp_Tm3100Led_Clear,
	.fill = Bsp_Tm3100Led_Fill,
	.refresh = Bsp_Tm3100Led_Refresh,
	.output_enable = Bsp_Tm3100Led_OutputEnable,
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
