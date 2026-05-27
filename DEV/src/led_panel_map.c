#include "led_panel.h"

#define LED_PANEL_POINT(chip_id, channel_id)                                             \
	{.chip = (uint8_t)(chip_id), .channel = (uint8_t)(channel_id)}

#define LED_PANEL_POINT_BY_NO(no)                                                        \
	LED_PANEL_POINT(((no) - 1u) / BSP_TM3100_CHANNEL_PER_CHIP,                           \
					((no) - 1u) % BSP_TM3100_CHANNEL_PER_CHIP)

static const stc_led_panel_point_t s_astLedPanelAbsPoints[] = {
	LED_PANEL_POINT_BY_NO(3u),
	LED_PANEL_POINT_BY_NO(19u),
};

const stc_led_panel_map_t g_astLedPanelMap[LedPanelIdCount] = {
	[LedPanelIdAbs] = {.points = s_astLedPanelAbsPoints,
					   .count = ARRAY_SZ(s_astLedPanelAbsPoints)},
};
