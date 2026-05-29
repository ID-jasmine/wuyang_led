#ifndef __LED_PANEL_H__
#define __LED_PANEL_H__

#include "bsp_tm3100_led.h"
#include "led_panel_map.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum en_led_panel_id
	{
		LedPanelIdAbs = 0u,
		LedPanelIdCount,
	} en_led_panel_id_t;

	typedef struct stc_led_panel_ops
	{
		en_result_t (*init)(void);
		en_result_t (*set)(en_led_panel_id_t led_id, boolean_t level);
		en_result_t (*set_chip_channel)(uint8_t chip, uint8_t channel, boolean_t level);
		en_result_t (*clear)(void);
		en_result_t (*fill)(void);
		en_result_t (*refresh)(void);
		en_result_t (*output_enable)(boolean_t enable);
	} stc_led_panel_ops_t;

	typedef struct stc_led_panel
	{
		const stc_led_panel_ops_t *ops;
		const stc_led_panel_map_t *map;
		uint16_t count;
	} stc_led_panel_t;

	extern stc_led_panel_t g_stcLedPanel;

	en_result_t LedPanel_Init(void);
	en_result_t LedPanel_Set(en_led_panel_id_t led_id, boolean_t level);
	en_result_t LedPanel_SetChipChannel(uint8_t chip, uint8_t channel, boolean_t level);
	en_result_t LedPanel_Clear(void);
	en_result_t LedPanel_Fill(void);
	en_result_t LedPanel_Refresh(void);
	en_result_t LedPanel_OutputEnable(boolean_t enable);

#ifdef __cplusplus
}
#endif

#endif
