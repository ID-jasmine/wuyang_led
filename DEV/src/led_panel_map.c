#include "led_panel.h"

#define LED_PANEL_POINT(chip_id, channel_id)                                             \
	{.chip = (uint8_t)(chip_id), .channel = (uint8_t)(channel_id)}

#define LED_PANEL_POINT_BY_INDEX(index)                                                  \
	LED_PANEL_POINT((index) / BSP_TM3100_CHANNEL_PER_CHIP,                                \
					(index) % BSP_TM3100_CHANNEL_PER_CHIP)

#define LED_PANEL_MAP_BY_POINTS(point_array)                                              \
	{.points = (point_array), .count = ARRAY_SZ(point_array)}

static const stc_led_panel_point_t s_astLedPanelBorderPoints[] = {
	LED_PANEL_POINT_BY_INDEX(14u),	LED_PANEL_POINT_BY_INDEX(30u),
	LED_PANEL_POINT_BY_INDEX(31u),	LED_PANEL_POINT_BY_INDEX(34u),
	LED_PANEL_POINT_BY_INDEX(35u),	LED_PANEL_POINT_BY_INDEX(36u),
	LED_PANEL_POINT_BY_INDEX(37u),	LED_PANEL_POINT_BY_INDEX(38u),
	LED_PANEL_POINT_BY_INDEX(39u),	LED_PANEL_POINT_BY_INDEX(40u),
	LED_PANEL_POINT_BY_INDEX(41u),	LED_PANEL_POINT_BY_INDEX(42u),
	LED_PANEL_POINT_BY_INDEX(43u),	LED_PANEL_POINT_BY_INDEX(44u),
	LED_PANEL_POINT_BY_INDEX(48u),	LED_PANEL_POINT_BY_INDEX(49u),
	LED_PANEL_POINT_BY_INDEX(50u),	LED_PANEL_POINT_BY_INDEX(56u),
	LED_PANEL_POINT_BY_INDEX(61u),	LED_PANEL_POINT_BY_INDEX(69u),
	LED_PANEL_POINT_BY_INDEX(70u),	LED_PANEL_POINT_BY_INDEX(71u),
	LED_PANEL_POINT_BY_INDEX(72u),	LED_PANEL_POINT_BY_INDEX(73u),
	LED_PANEL_POINT_BY_INDEX(85u),	LED_PANEL_POINT_BY_INDEX(86u),
	LED_PANEL_POINT_BY_INDEX(87u),	LED_PANEL_POINT_BY_INDEX(88u),
	LED_PANEL_POINT_BY_INDEX(89u),	LED_PANEL_POINT_BY_INDEX(96u),
	LED_PANEL_POINT_BY_INDEX(104u), LED_PANEL_POINT_BY_INDEX(105u),
	LED_PANEL_POINT_BY_INDEX(106u), LED_PANEL_POINT_BY_INDEX(107u),
	LED_PANEL_POINT_BY_INDEX(108u), LED_PANEL_POINT_BY_INDEX(109u),
	LED_PANEL_POINT_BY_INDEX(110u), LED_PANEL_POINT_BY_INDEX(111u),
	LED_PANEL_POINT_BY_INDEX(122u), LED_PANEL_POINT_BY_INDEX(123u),
	LED_PANEL_POINT_BY_INDEX(124u), LED_PANEL_POINT_BY_INDEX(125u),
	LED_PANEL_POINT_BY_INDEX(126u), LED_PANEL_POINT_BY_INDEX(127u),
	LED_PANEL_POINT_BY_INDEX(128u), LED_PANEL_POINT_BY_INDEX(129u),
	LED_PANEL_POINT_BY_INDEX(130u), LED_PANEL_POINT_BY_INDEX(131u),
	LED_PANEL_POINT_BY_INDEX(132u), LED_PANEL_POINT_BY_INDEX(133u),
	LED_PANEL_POINT_BY_INDEX(134u), LED_PANEL_POINT_BY_INDEX(135u),
	LED_PANEL_POINT_BY_INDEX(136u), LED_PANEL_POINT_BY_INDEX(137u),
	LED_PANEL_POINT_BY_INDEX(138u), LED_PANEL_POINT_BY_INDEX(139u),
	LED_PANEL_POINT_BY_INDEX(141u), LED_PANEL_POINT_BY_INDEX(142u),
	LED_PANEL_POINT_BY_INDEX(143u), LED_PANEL_POINT_BY_INDEX(150u),
	LED_PANEL_POINT_BY_INDEX(151u), LED_PANEL_POINT_BY_INDEX(152u),
	LED_PANEL_POINT_BY_INDEX(153u), LED_PANEL_POINT_BY_INDEX(154u),
	LED_PANEL_POINT_BY_INDEX(155u), LED_PANEL_POINT_BY_INDEX(222u),
	LED_PANEL_POINT_BY_INDEX(223u),
};

static const stc_led_panel_point_t s_astLedPanelHighBeamPoints[] = {
	LED_PANEL_POINT_BY_INDEX(32u),
};
static const stc_led_panel_point_t s_astLedPanelBatteryFaultPoints[] = {
	LED_PANEL_POINT_BY_INDEX(33u),
};
static const stc_led_panel_point_t s_astLedPanelEngineFaultPoints[] = {
	LED_PANEL_POINT_BY_INDEX(45u),
};
static const stc_led_panel_point_t s_astLedPanelGearNPoints[] = {
	LED_PANEL_POINT_BY_INDEX(46u), LED_PANEL_POINT_BY_INDEX(47u),
};
static const stc_led_panel_point_t s_astLedPanelLeftTurnPoints[] = {
	LED_PANEL_POINT_BY_INDEX(62u), LED_PANEL_POINT_BY_INDEX(63u),
};
static const stc_led_panel_point_t s_astLedPanelRightTurnPoints[] = {
	LED_PANEL_POINT_BY_INDEX(90u), LED_PANEL_POINT_BY_INDEX(91u),
};
static const stc_led_panel_point_t s_astLedPanelFuelWhitePoints[] = {
	LED_PANEL_POINT_BY_INDEX(120u),
};
static const stc_led_panel_point_t s_astLedPanelFuelYellowPoints[] = {
	LED_PANEL_POINT_BY_INDEX(121u),
};
static const stc_led_panel_point_t s_astLedPanelMileageMilePoints[] = {
	LED_PANEL_POINT_BY_INDEX(140u),
};
static const stc_led_panel_point_t s_astLedPanelSpeedKmPoints[] = {
	LED_PANEL_POINT_BY_INDEX(144u), LED_PANEL_POINT_BY_INDEX(145u),
};
static const stc_led_panel_point_t s_astLedPanelSpeedMphPoints[] = {
	LED_PANEL_POINT_BY_INDEX(146u), LED_PANEL_POINT_BY_INDEX(147u),
};
static const stc_led_panel_point_t s_astLedPanelOilWarningPoints[] = {
	LED_PANEL_POINT_BY_INDEX(148u), LED_PANEL_POINT_BY_INDEX(149u),
};
static const stc_led_panel_point_t s_astLedPanelAbsPoints[] = {
	LED_PANEL_POINT_BY_INDEX(156u), LED_PANEL_POINT_BY_INDEX(157u),
};
static const stc_led_panel_point_t s_astLedPanelWaterTempPoints[] = {
	LED_PANEL_POINT_BY_INDEX(158u), LED_PANEL_POINT_BY_INDEX(159u),
};
static const stc_led_panel_point_t s_astLedPanelOdoPoints[] = {
	LED_PANEL_POINT_BY_INDEX(206u),
};
static const stc_led_panel_point_t s_astLedPanelTripPoints[] = {
	LED_PANEL_POINT_BY_INDEX(207u),
};
static const stc_led_panel_point_t s_astLedPanelMileageKmPoints[] = {
	LED_PANEL_POINT_BY_INDEX(238u),
};
static const stc_led_panel_point_t s_astLedPanelClockColonPoints[] = {
	LED_PANEL_POINT_BY_INDEX(15u),
};

const uint8_t g_au8LedPanelClockHourTensSegments[LED_PANEL_SEGMENT_COUNT] = {
	0u, 1u, 2u, 3u, 4u, 5u, 6u,
};
const uint8_t g_au8LedPanelClockHourOnesSegments[LED_PANEL_SEGMENT_COUNT] = {
	7u, 8u, 9u, 10u, 11u, 12u, 13u,
};
const uint8_t g_au8LedPanelClockMinuteTensSegments[LED_PANEL_SEGMENT_COUNT] = {
	16u, 17u, 18u, 19u, 20u, 21u, 22u,
};
const uint8_t g_au8LedPanelClockMinuteOnesSegments[LED_PANEL_SEGMENT_COUNT] = {
	23u, 24u, 25u, 26u, 27u, 28u, 29u,
};
const uint8_t g_au8LedPanelGearSegments[LED_PANEL_SEGMENT_COUNT] = {
	103u, 102u, 100u, 99u, 97u, 98u, 101u,
};
const uint8_t g_au8LedPanelOdoTenThousandsSegments[LED_PANEL_SEGMENT_COUNT] = {
	192u, 193u, 194u, 195u, 196u, 197u, 198u,
};
const uint8_t g_au8LedPanelOdoThousandsSegments[LED_PANEL_SEGMENT_COUNT] = {
	199u, 200u, 201u, 202u, 203u, 204u, 205u,
};
const uint8_t g_au8LedPanelOdoHundredsSegments[LED_PANEL_SEGMENT_COUNT] = {
	208u, 209u, 210u, 211u, 212u, 213u, 214u,
};
const uint8_t g_au8LedPanelOdoTensSegments[LED_PANEL_SEGMENT_COUNT] = {
	215u, 216u, 217u, 218u, 219u, 220u, 221u,
};
const uint8_t g_au8LedPanelOdoOnesSegments[LED_PANEL_SEGMENT_COUNT] = {
	224u, 225u, 226u, 227u, 228u, 229u, 230u,
};
const uint8_t g_au8LedPanelOdoDecimalSegments[LED_PANEL_SEGMENT_COUNT] = {
	231u, 232u, 233u, 234u, 235u, 236u, 237u,
};
const uint8_t g_u8LedPanelOdoDecimalPointIndex = 239u;

static const uint8_t s_au8LedPanelSpeedOnesSegA[] = {168u, 169u};
static const uint8_t s_au8LedPanelSpeedOnesSegB[] = {170u, 171u};
static const uint8_t s_au8LedPanelSpeedOnesSegC[] = {172u, 173u};
static const uint8_t s_au8LedPanelSpeedOnesSegD[] = {174u, 175u};
static const uint8_t s_au8LedPanelSpeedOnesSegE[] = {164u, 165u};
static const uint8_t s_au8LedPanelSpeedOnesSegF[] = {160u, 161u};
static const uint8_t s_au8LedPanelSpeedOnesSegG[] = {162u, 163u};
static const uint8_t s_au8LedPanelSpeedTensSegA[] = {184u, 185u};
static const uint8_t s_au8LedPanelSpeedTensSegB[] = {186u, 187u};
static const uint8_t s_au8LedPanelSpeedTensSegC[] = {188u, 189u};
static const uint8_t s_au8LedPanelSpeedTensSegD[] = {190u, 191u};
static const uint8_t s_au8LedPanelSpeedTensSegE[] = {180u, 181u};
static const uint8_t s_au8LedPanelSpeedTensSegF[] = {176u, 177u};
static const uint8_t s_au8LedPanelSpeedTensSegG[] = {178u, 179u};

const uint8_t g_au8LedPanelSpeedHundredsSegB[LED_PANEL_SPEED_HUNDREDS_SEG_COUNT] = {
	182u, 183u,
};
const uint8_t g_au8LedPanelSpeedHundredsSegC[LED_PANEL_SPEED_HUNDREDS_SEG_COUNT] = {
	166u, 167u,
};

const stc_led_panel_index_group_t
	g_astLedPanelSpeedOnesSegments[LED_PANEL_SEGMENT_COUNT] = {
		{s_au8LedPanelSpeedOnesSegA, ARRAY_SZ(s_au8LedPanelSpeedOnesSegA)},
		{s_au8LedPanelSpeedOnesSegB, ARRAY_SZ(s_au8LedPanelSpeedOnesSegB)},
		{s_au8LedPanelSpeedOnesSegC, ARRAY_SZ(s_au8LedPanelSpeedOnesSegC)},
		{s_au8LedPanelSpeedOnesSegD, ARRAY_SZ(s_au8LedPanelSpeedOnesSegD)},
		{s_au8LedPanelSpeedOnesSegE, ARRAY_SZ(s_au8LedPanelSpeedOnesSegE)},
		{s_au8LedPanelSpeedOnesSegF, ARRAY_SZ(s_au8LedPanelSpeedOnesSegF)},
		{s_au8LedPanelSpeedOnesSegG, ARRAY_SZ(s_au8LedPanelSpeedOnesSegG)},
};
const stc_led_panel_index_group_t
	g_astLedPanelSpeedTensSegments[LED_PANEL_SEGMENT_COUNT] = {
		{s_au8LedPanelSpeedTensSegA, ARRAY_SZ(s_au8LedPanelSpeedTensSegA)},
		{s_au8LedPanelSpeedTensSegB, ARRAY_SZ(s_au8LedPanelSpeedTensSegB)},
		{s_au8LedPanelSpeedTensSegC, ARRAY_SZ(s_au8LedPanelSpeedTensSegC)},
		{s_au8LedPanelSpeedTensSegD, ARRAY_SZ(s_au8LedPanelSpeedTensSegD)},
		{s_au8LedPanelSpeedTensSegE, ARRAY_SZ(s_au8LedPanelSpeedTensSegE)},
		{s_au8LedPanelSpeedTensSegF, ARRAY_SZ(s_au8LedPanelSpeedTensSegF)},
		{s_au8LedPanelSpeedTensSegG, ARRAY_SZ(s_au8LedPanelSpeedTensSegG)},
};

const uint8_t g_au8LedPanelRpmBarIndices[LED_PANEL_RPM_BAR_COUNT] = {
	51u, 52u, 53u, 54u, 55u, 60u, 59u, 58u, 57u, 68u, 67u, 66u, 65u, 64u, 79u,
	78u, 77u, 76u, 75u, 74u, 84u, 83u, 82u, 81u, 80u, 95u, 92u, 93u, 94u,
};
const uint8_t g_au8LedPanelFuelBarIndices[LED_PANEL_FUEL_BAR_COUNT] = {
	112u, 113u, 114u, 115u, 116u, 119u, 118u, 117u,
};

const stc_led_panel_map_t g_astLedPanelMap[LedPanelIdCount] = {
	[LedPanelIdBorder] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelBorderPoints),
	[LedPanelIdHighBeam] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelHighBeamPoints),
	[LedPanelIdBatteryFault] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelBatteryFaultPoints),
	[LedPanelIdEngineFault] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelEngineFaultPoints),
	[LedPanelIdGearN] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelGearNPoints),
	[LedPanelIdLeftTurn] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelLeftTurnPoints),
	[LedPanelIdRightTurn] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelRightTurnPoints),
	[LedPanelIdFuelWhite] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelFuelWhitePoints),
	[LedPanelIdFuelYellow] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelFuelYellowPoints),
	[LedPanelIdMileageMile] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelMileageMilePoints),
	[LedPanelIdSpeedKm] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelSpeedKmPoints),
	[LedPanelIdSpeedMph] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelSpeedMphPoints),
	[LedPanelIdOilWarning] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelOilWarningPoints),
	[LedPanelIdAbs] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelAbsPoints),
	[LedPanelIdWaterTemp] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelWaterTempPoints),
	[LedPanelIdOdo] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelOdoPoints),
	[LedPanelIdTrip] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelTripPoints),
	[LedPanelIdMileageKm] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelMileageKmPoints),
	[LedPanelIdClockColon] = LED_PANEL_MAP_BY_POINTS(s_astLedPanelClockColonPoints),
};
