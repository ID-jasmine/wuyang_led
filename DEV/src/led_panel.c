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
 * @brief  LED面板模块初始化
 * @return en_result_t Ok: 初始化成功
 */
en_result_t LedPanel_Init(void)
{
	return g_stcLedPanel.ops->init();
}

/**
 * @brief  控制指定逻辑LED的点亮/熄灭（按逻辑ID映射底层驱动点）
 * @param  led_id 逻辑LED面板ID
 * @param  level  控制电平 (TRUE为亮，FALSE为灭)
 * @return en_result_t Ok: 操作成功
 */
en_result_t LedPanel_Set(en_led_panel_id_t led_id, boolean_t level)
{
	return g_stcLedPanel.ops->set(led_id, level);
}

/**
 * @brief  直接控制指定驱动芯片的指定通道状态
 * @param  chip    芯片序号
 * @param  channel 通道序号
 * @param  level   控制电平 (TRUE为亮，FALSE为灭)
 * @return en_result_t Ok: 操作成功
 */
en_result_t LedPanel_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t level)
{
	return g_stcLedPanel.ops->set_chip_channel(chip, channel, level);
}

/**
 * @brief  清空所有LED缓存数据（准备全灭）
 * @return en_result_t Ok: 操作成功
 */
en_result_t LedPanel_Clear(void)
{
	return g_stcLedPanel.ops->clear();
}

/**
 * @brief  填充所有LED缓存数据（准备全亮）
 * @return en_result_t Ok: 操作成功
 */
en_result_t LedPanel_Fill(void)
{
	return g_stcLedPanel.ops->fill();
}

/**
 * @brief  将缓存中的LED状态刷新并发送到底层物理芯片
 * @return en_result_t Ok: 操作成功
 */
en_result_t LedPanel_Refresh(void)
{
	return g_stcLedPanel.ops->refresh();
}

/**
 * @brief  LED驱动芯片全局输出使能/禁能控制
 * @param  enable TRUE 打开全局输出，FALSE 关闭全局输出
 * @return en_result_t Ok: 操作成功
 */
en_result_t LedPanel_OutputEnable(boolean_t enable)
{
	return g_stcLedPanel.ops->output_enable(enable);
}
