#ifndef __LED_PANEL_H__
#define __LED_PANEL_H__

#include "bsp_tm3100_led.h"
#include "led_panel_map.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		LedPanelIdBorder = 0u,
		LedPanelIdHighBeam,
		LedPanelIdBatteryFault,
		LedPanelIdEngineFault,
		LedPanelIdGearN,
		LedPanelIdLeftTurn,
		LedPanelIdRightTurn,
		LedPanelIdFuelWhite,
		LedPanelIdFuelYellow,
		LedPanelIdMileageMile,
		LedPanelIdSpeedKm,
		LedPanelIdSpeedMph,
		LedPanelIdOilWarning,
		LedPanelIdAbs,
		LedPanelIdWaterTemp,
		LedPanelIdOdo,
		LedPanelIdTrip,
		LedPanelIdMileageKm,
		LedPanelIdClockColon,
		LedPanelIdCount,
	} en_led_panel_id_t;

	typedef struct
	{
		en_result_t (*init)(void);
		en_result_t (*set)(en_led_panel_id_t led_id, boolean_t level);
		en_result_t (*set_chip_channel)(uint8_t chip, uint8_t channel, boolean_t level);
		en_result_t (*clear)(void);
		en_result_t (*fill)(void);
		en_result_t (*refresh)(void);
		en_result_t (*output_enable)(boolean_t enable);
		en_result_t (*set_brightness)(uint8_t brightness_percent);
		uint8_t (*get_brightness)(void);
	} stc_led_panel_ops_t;

	typedef struct
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
	en_result_t LedPanel_SetBrightness(uint8_t brightness_percent);
	uint8_t LedPanel_GetBrightness(void);
	en_result_t LedPanel_SetBorder(boolean_t level);
	en_result_t LedPanel_ShowClock(uint8_t hour, uint8_t minute);
	en_result_t LedPanel_ShowGearDigit(uint8_t digit);
	en_result_t LedPanel_ClearGearDigit(void);
	en_result_t LedPanel_ShowSpeed(uint16_t speed);
	en_result_t LedPanel_ShowSpeedDigits(uint8_t tens_digit, uint8_t ones_digit,
										 boolean_t hundreds_on);
	en_result_t LedPanel_ShowOdometer(uint32_t value, uint8_t decimal_digit);
	en_result_t LedPanel_ShowTripOdometer(uint16_t tenths_km);
	en_result_t LedPanel_ShowTotalOdometer(uint32_t km);
	en_result_t LedPanel_ShowRpmBars(uint8_t bar_count);
	en_result_t LedPanel_ShowRpmPercent(uint8_t percent);
	en_result_t LedPanel_ShowFuelBars(uint8_t bar_count);
	en_result_t LedPanel_ShowFuelPercent(uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif
