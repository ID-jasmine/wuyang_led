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
		enRet = Bsp_Tm3100Led_SetChipChannel(pstcMap->points[i].chip, pstcMap->points[i].channel, level);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

static const stc_led_panel_ops_t s_stcLedPanelOps = {
	Bsp_Tm3100Led_Init,			LedPanel_SetImpl,	Bsp_Tm3100Led_SetChipChannel,
	Bsp_Tm3100Led_Clear,		Bsp_Tm3100Led_Fill, Bsp_Tm3100Led_Refresh,
	Bsp_Tm3100Led_OutputEnable,
};

stc_led_panel_t g_stcLedPanel = {
	&s_stcLedPanelOps,
	g_astLedPanelMap,
	(uint16_t)LedPanelIdCount,
};

en_result_t LedPanel_Init(void)
{
	return g_stcLedPanel.ops->init();
}

en_result_t LedPanel_Set(en_led_panel_id_t led_id, boolean_t level)
{
	return g_stcLedPanel.ops->set(led_id, level);
}

en_result_t LedPanel_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t level)
{
	return g_stcLedPanel.ops->set_chip_channel(chip, channel, level);
}

en_result_t LedPanel_Clear(void)
{
	return g_stcLedPanel.ops->clear();
}

en_result_t LedPanel_Fill(void)
{
	return g_stcLedPanel.ops->fill();
}

en_result_t LedPanel_Refresh(void)
{
	return g_stcLedPanel.ops->refresh();
}

en_result_t LedPanel_OutputEnable(boolean_t enable)
{
	return g_stcLedPanel.ops->output_enable(enable);
}
