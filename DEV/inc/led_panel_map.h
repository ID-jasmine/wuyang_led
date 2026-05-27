#ifndef __LED_PANEL_MAP_H__
#define __LED_PANEL_MAP_H__

#include "bsp_tm3100_led.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct stc_led_panel_point
	{
		uint8_t chip;
		uint8_t channel;
	} stc_led_panel_point_t;

	typedef struct stc_led_panel_map
	{
		const stc_led_panel_point_t *points;
		uint8_t count;
	} stc_led_panel_map_t;

	extern const stc_led_panel_map_t g_astLedPanelMap[];

#ifdef __cplusplus
}
#endif

#endif
