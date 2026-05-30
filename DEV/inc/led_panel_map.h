#ifndef __LED_PANEL_MAP_H__
#define __LED_PANEL_MAP_H__

#include "bsp_tm3100_led.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LED_PANEL_SEGMENT_COUNT	  (7u)
#define LED_PANEL_SPEED_HUNDREDS_SEG_COUNT (2u)
#define LED_PANEL_RPM_BAR_COUNT	  (29u)
#define LED_PANEL_FUEL_BAR_COUNT  (8u)

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

	typedef struct stc_led_panel_index_group
	{
		const uint8_t *indices;
		uint8_t count;
	} stc_led_panel_index_group_t;

	extern const stc_led_panel_map_t g_astLedPanelMap[];
	extern const uint8_t g_au8LedPanelClockHourTensSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelClockHourOnesSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelClockMinuteTensSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelClockMinuteOnesSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelGearSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelOdoTenThousandsSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelOdoThousandsSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelOdoHundredsSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelOdoTensSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelOdoOnesSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_au8LedPanelOdoDecimalSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t g_u8LedPanelOdoDecimalPointIndex;
	extern const stc_led_panel_index_group_t
		g_astLedPanelSpeedOnesSegments[LED_PANEL_SEGMENT_COUNT];
	extern const stc_led_panel_index_group_t
		g_astLedPanelSpeedTensSegments[LED_PANEL_SEGMENT_COUNT];
	extern const uint8_t
		g_au8LedPanelSpeedHundredsSegB[LED_PANEL_SPEED_HUNDREDS_SEG_COUNT];
	extern const uint8_t
		g_au8LedPanelSpeedHundredsSegC[LED_PANEL_SPEED_HUNDREDS_SEG_COUNT];
	extern const uint8_t g_au8LedPanelRpmBarIndices[LED_PANEL_RPM_BAR_COUNT];
	extern const uint8_t g_au8LedPanelFuelBarIndices[LED_PANEL_FUEL_BAR_COUNT];

#ifdef __cplusplus
}
#endif

#endif
