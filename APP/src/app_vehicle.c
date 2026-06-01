#include "app_vehicle.h"

#include "bsp_tm3100_led.h"
#include "dev_speed_rpm.h"
#include "drv_adc.h"
#include "drv_button.h"
#include "drv_eeprom.h"
#include "drv_input.h"
#include "drv_rtc.h"
#include "led_panel.h"

#define APP_VEHICLE_SELF_CHECK_SETUP_TICKS	  (1u)
#define APP_VEHICLE_SELF_CHECK_RPM_DOWN_TICKS (100u)
#define APP_VEHICLE_SELF_CHECK_FULL_ON_TICKS  (1u)
#define APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS  (100u)
#define APP_VEHICLE_SELF_CHECK_TICKS                                                     \
	(APP_VEHICLE_SELF_CHECK_SETUP_TICKS + APP_VEHICLE_SELF_CHECK_RPM_DOWN_TICKS +        \
	 APP_VEHICLE_SELF_CHECK_FULL_ON_TICKS + APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS)
#define APP_VEHICLE_SELF_CHECK_SPEED_DIGIT_TICKS (8u)
#define APP_VEHICLE_SELF_CHECK_SWEEP_TICKS		 (4u)
#define APP_VEHICLE_NORMAL_REFRESH_TICKS		 (10u)
#define APP_VEHICLE_ADC_FULL_SCALE				 (4095u)
#define APP_VEHICLE_SPEED_PULSES_PER_KM			 (2800u)
#define APP_VEHICLE_MILLIHZ_PER_HZ				 (1000u)
#define APP_VEHICLE_HALL_POLE_PAIRS				 (2u)
#define APP_VEHICLE_RPM_RATIO_NUMERATOR			 (667u)
#define APP_VEHICLE_RPM_RATIO_DENOMINATOR		 (100u)
#define APP_VEHICLE_RPM_PER_BAR					 (500u)
#define APP_VEHICLE_DISPLAY_CONFIRM_TICKS		 (2u)
#define APP_VEHICLE_SPEED_DISPLAY_STEP			 (3u)
#define APP_VEHICLE_GEAR_BLINK_TICKS			 (5u)
#define APP_VEHICLE_FUEL_FAST_TICKS				 (30u)
#define APP_VEHICLE_FUEL_SLOW_TICKS				 (300u)
#define APP_VEHICLE_FUEL_INVALID_BARS			 (0u)
#define APP_VEHICLE_FUEL_RES_CORRECT_NUMERATOR	 (94u)
#define APP_VEHICLE_FUEL_RES_CORRECT_DENOMINATOR (100u)
#define APP_VEHICLE_ODOMETER_PULSES_PER_KM		 (2800u)
#define APP_VEHICLE_ODOMETER_PULSES_PER_TENTH  (APP_VEHICLE_ODOMETER_PULSES_PER_KM / 10u)
#define APP_VEHICLE_TRIP_MAX_TENTHS			   (9999u)
#define APP_VEHICLE_TOTAL_MAX_TENTHS		   (9999990u)
#define APP_VEHICLE_MILEAGE_EEPROM_ADDR		   (0u)
#define APP_VEHICLE_MILEAGE_MAGIC			   (0x4F444F31u)
#define APP_VEHICLE_MILEAGE_VERSION			   (1u)
#define APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO (0u)
#define APP_VEHICLE_FREQ_MEASURE			   DevSpeedRpmMeasureGate

#if (APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO != 0u)
#include "bsp_sys.h"
#endif

typedef struct
{
	uint32_t magic;
	uint32_t total_tenths_km;
	uint16_t trip_tenths_km;
	uint8_t unit_imperial;
	uint8_t reserved;
	uint8_t version;
	uint8_t display_trip;
	uint16_t checksum;
} stc_app_vehicle_mileage_store_t;

typedef struct
{
	uint16_t input;
	uint16_t output;
} stc_app_vehicle_speed_map_t;

static boolean_t s_bVehicleSelfCheckStarted = FALSE;
static uint16_t s_u16VehicleSelfCheckTick = 0u;
static uint8_t s_u8VehicleNormalRefreshTick = 0u;
static boolean_t s_bVehicleMileageLoaded = FALSE;
static boolean_t s_bVehicleMileageTripDisplay = FALSE;
static uint32_t s_u32VehicleMileageLastPulseCount = 0u;
static uint32_t s_u32VehicleMileagePulseRemainder = 0u;
static uint32_t s_u32VehicleTotalTenthsKm = 0u;
static uint16_t s_u16VehicleTripTenthsKm = 0u;
static boolean_t s_bVehicleUnitImperial = FALSE;
static boolean_t s_bVehicleFuelInited = FALSE;
static uint8_t s_u8VehicleFuelDisplayBars = 0u;
static uint16_t s_u16VehicleFuelFastTick = 0u;
static uint16_t s_u16VehicleFuelSlowTick = 0u;
static uint8_t s_u8VehicleGearBlinkTick = 0u;
static boolean_t s_bVehicleGearBlinkOn = TRUE;
static uint16_t s_u16VehicleDisplaySpeed = 0u;
static uint16_t s_u16VehicleSpeedCandidate = 0u;
static uint8_t s_u8VehicleSpeedCandidateTicks = 0u;
static boolean_t s_bVehicleSpeedDisplayInited = FALSE;
static uint8_t s_u8VehicleDisplayRpmBars = 0u;
static uint8_t s_u8VehicleRpmBarsCandidate = 0u;
static uint8_t s_u8VehicleRpmBarsCandidateTicks = 0u;
static boolean_t s_bVehicleRpmBarsDisplayInited = FALSE;
#if (APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO != 0u)
static uint32_t s_u32VehicleTestLastSpeedPulseCount = 0u;
static uint32_t s_u32VehicleTestLastRpmPulseCount = 0u;
static uint32_t s_u32VehicleTestGateStartMs = 0u;
static uint32_t s_u32VehicleTestFreqX100 = 0u;
static boolean_t s_bVehicleTestGateInited = FALSE;
#endif

static const uint8_t s_au8VehicleDigitPattern[10] = {
	(uint8_t)(0x01u | 0x02u | 0x04u | 0x08u | 0x10u | 0x20u),
	(uint8_t)(0x02u | 0x04u),
	(uint8_t)(0x01u | 0x02u | 0x08u | 0x10u | 0x40u),
	(uint8_t)(0x01u | 0x02u | 0x04u | 0x08u | 0x40u),
	(uint8_t)(0x02u | 0x04u | 0x20u | 0x40u),
	(uint8_t)(0x01u | 0x04u | 0x08u | 0x20u | 0x40u),
	(uint8_t)(0x01u | 0x04u | 0x08u | 0x10u | 0x20u | 0x40u),
	(uint8_t)(0x01u | 0x02u | 0x04u),
	(uint8_t)(0x01u | 0x02u | 0x04u | 0x08u | 0x10u | 0x20u | 0x40u),
	(uint8_t)(0x01u | 0x02u | 0x04u | 0x08u | 0x20u | 0x40u),
};

static const stc_app_vehicle_speed_map_t s_astVehicleSpeedMap[] = {
	{0u, 0u},	{10u, 11u},	  {20u, 21u},	{40u, 43u},	  {60u, 66u},
	{80u, 86u}, {100u, 108u}, {120u, 131u}, {140u, 151u}, {199u, 199u},
};

/* Clock state */
static uint8_t s_u8VehicleClockHour = 0u;
static uint8_t s_u8VehicleClockMinute = 0u;
static boolean_t s_bVehicleClockValid = FALSE;
static boolean_t s_bVehicleClockSettingMode = FALSE;
static boolean_t s_bVehicleClockSettingHour = TRUE;
static boolean_t s_bVehicleClockBlinkState = FALSE;
static uint8_t s_u8VehicleClockBlinkTick = 0;

static en_result_t App_Vehicle_SetLinearList(const uint8_t *indices, uint8_t count,
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

static en_result_t App_Vehicle_SetSegmentGroupPattern(
	const stc_led_panel_index_group_t groups[LED_PANEL_SEGMENT_COUNT], uint8_t pattern)
{
	uint8_t i;
	en_result_t enRet;

	if (NULL == groups)
	{
		return ErrorInvalidParameter;
	}

	for (i = 0u; i < LED_PANEL_SEGMENT_COUNT; i++)
	{
		enRet =
			App_Vehicle_SetLinearList(groups[i].indices, groups[i].count,
									  (boolean_t)(0u != (pattern & (uint8_t)(1u << i))));
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

static uint8_t App_Vehicle_GetDigitPattern(uint8_t digit)
{
	if (digit > 9u)
	{
		return 0u;
	}

	return s_au8VehicleDigitPattern[digit];
}

static void App_Vehicle_ShowSelfCheckSpeedRaw(uint8_t tens_digit, uint8_t ones_digit,
											  boolean_t hundreds_on)
{
	App_Vehicle_SetLinearList(g_au8LedPanelSpeedHundredsSegB,
							  ARRAY_SZ(g_au8LedPanelSpeedHundredsSegB), hundreds_on);
	App_Vehicle_SetLinearList(g_au8LedPanelSpeedHundredsSegC,
							  ARRAY_SZ(g_au8LedPanelSpeedHundredsSegC), hundreds_on);
	App_Vehicle_SetSegmentGroupPattern(g_astLedPanelSpeedTensSegments,
									   App_Vehicle_GetDigitPattern(tens_digit));
	App_Vehicle_SetSegmentGroupPattern(g_astLedPanelSpeedOnesSegments,
									   App_Vehicle_GetDigitPattern(ones_digit));
}

static void App_Vehicle_ShowSelfCheckSpeedSweep(uint16_t tick)
{
	static const uint8_t au8SweepPatterns[] = {
		(uint8_t)(0x02u | 0x04u), (uint8_t)(0x01u | 0x02u), (uint8_t)(0x20u | 0x01u),
		(uint8_t)(0x10u | 0x20u), (uint8_t)(0x08u | 0x10u), (uint8_t)(0x04u | 0x08u),
	};
	uint8_t pattern;

	pattern = au8SweepPatterns[(tick / APP_VEHICLE_SELF_CHECK_SWEEP_TICKS) %
							   ARRAY_SZ(au8SweepPatterns)];
	App_Vehicle_SetLinearList(g_au8LedPanelSpeedHundredsSegB,
							  ARRAY_SZ(g_au8LedPanelSpeedHundredsSegB), FALSE);
	App_Vehicle_SetLinearList(g_au8LedPanelSpeedHundredsSegC,
							  ARRAY_SZ(g_au8LedPanelSpeedHundredsSegC), FALSE);
	App_Vehicle_SetSegmentGroupPattern(g_astLedPanelSpeedTensSegments, pattern);
	App_Vehicle_SetSegmentGroupPattern(g_astLedPanelSpeedOnesSegments, pattern);
}

static uint16_t App_Vehicle_ConfirmDisplayU16(uint16_t sample, uint16_t *display,
											  uint16_t *candidate,
											  uint8_t *candidate_ticks, boolean_t *inited)
{
	uint16_t target;
	uint16_t step;

	if (FALSE == *inited)
	{
		*display = sample;
		*candidate = sample;
		*candidate_ticks = 0u;
		*inited = TRUE;
		return *display;
	}

	if (sample == *display)
	{
		*candidate = sample;
		*candidate_ticks = 0u;
		return *display;
	}

	if (sample != *candidate)
	{
		*candidate = sample;
		*candidate_ticks = 1u;
	}
	else if (*candidate_ticks < APP_VEHICLE_DISPLAY_CONFIRM_TICKS)
	{
		(*candidate_ticks)++;
	}

	if (*candidate_ticks >= APP_VEHICLE_DISPLAY_CONFIRM_TICKS)
	{
		target = *candidate;
		if (*display < target)
		{
			step = (uint16_t)(target - *display);
			if (step > APP_VEHICLE_SPEED_DISPLAY_STEP)
			{
				step = APP_VEHICLE_SPEED_DISPLAY_STEP;
			}
			*display = (uint16_t)(*display + step);
		}
		else if (*display > target)
		{
			step = (uint16_t)(*display - target);
			if (step > APP_VEHICLE_SPEED_DISPLAY_STEP)
			{
				step = APP_VEHICLE_SPEED_DISPLAY_STEP;
			}
			*display = (uint16_t)(*display - step);
		}
		*candidate_ticks = APP_VEHICLE_DISPLAY_CONFIRM_TICKS;
	}

	return *display;
}

static uint8_t App_Vehicle_ConfirmDisplayU8(uint8_t sample, uint8_t *display,
											uint8_t *candidate, uint8_t *candidate_ticks,
											boolean_t *inited)
{
	uint8_t target;

	if (FALSE == *inited)
	{
		*display = sample;
		*candidate = sample;
		*candidate_ticks = 0u;
		*inited = TRUE;
		return *display;
	}

	if (sample == *display)
	{
		*candidate = sample;
		*candidate_ticks = 0u;
		return *display;
	}

	if (sample != *candidate)
	{
		*candidate = sample;
		*candidate_ticks = 1u;
	}
	else if (*candidate_ticks < APP_VEHICLE_DISPLAY_CONFIRM_TICKS)
	{
		(*candidate_ticks)++;
	}

	if (*candidate_ticks >= APP_VEHICLE_DISPLAY_CONFIRM_TICKS)
	{
		target = *candidate;
		if (*display < target)
		{
			(*display)++;
		}
		else if (*display > target)
		{
			(*display)--;
		}
		*candidate_ticks = APP_VEHICLE_DISPLAY_CONFIRM_TICKS;
	}

	return *display;
}

static uint16_t App_Vehicle_MapSpeed(uint16_t speed)
{
	uint8_t i;
	const stc_app_vehicle_speed_map_t *lower;
	const stc_app_vehicle_speed_map_t *upper;
	uint32_t numerator;
	uint32_t denominator;
	uint32_t mapped;

	for (i = 1u; i < ARRAY_SZ(s_astVehicleSpeedMap); i++)
	{
		upper = &s_astVehicleSpeedMap[i];
		if (speed <= upper->input)
		{
			lower = &s_astVehicleSpeedMap[i - 1u];
			numerator = (uint32_t)(speed - lower->input) *
						(uint32_t)(upper->output - lower->output);
			denominator = (uint32_t)(upper->input - lower->input);
			mapped = (uint32_t)lower->output +
					 ((numerator + (denominator / 2u)) / denominator);
			return (uint16_t)mapped;
		}
	}

	return s_astVehicleSpeedMap[ARRAY_SZ(s_astVehicleSpeedMap) - 1u].output;
}

static uint16_t
App_Vehicle_CalcMileageChecksum(const stc_app_vehicle_mileage_store_t *store)
{
	const uint8_t *bytes;
	uint16_t checksum;
	uint8_t i;

	bytes = (const uint8_t *)store;
	checksum = 0u;
	for (i = 0u; i < (uint8_t)(sizeof(*store) - sizeof(store->checksum)); i++)
	{
		checksum = (uint16_t)(checksum + bytes[i]);
	}

	return (uint16_t)(~checksum);
}

static boolean_t
App_Vehicle_IsMileageStoreValid(const stc_app_vehicle_mileage_store_t *store)
{
	if ((APP_VEHICLE_MILEAGE_MAGIC != store->magic) ||
		(APP_VEHICLE_MILEAGE_VERSION != store->version) ||
		(store->total_tenths_km > APP_VEHICLE_TOTAL_MAX_TENTHS) ||
		(store->trip_tenths_km > APP_VEHICLE_TRIP_MAX_TENTHS))
	{
		return FALSE;
	}

	return (store->checksum == App_Vehicle_CalcMileageChecksum(store)) ? TRUE : FALSE;
}

static void App_Vehicle_SaveMileage(void)
{
	stc_app_vehicle_mileage_store_t store;

	store.magic = APP_VEHICLE_MILEAGE_MAGIC;
	store.total_tenths_km = s_u32VehicleTotalTenthsKm;
	store.trip_tenths_km = s_u16VehicleTripTenthsKm;
	store.unit_imperial = (TRUE == s_bVehicleUnitImperial) ? 1u : 0u;
	store.reserved = 0u;
	store.version = APP_VEHICLE_MILEAGE_VERSION;
	store.display_trip = (TRUE == s_bVehicleMileageTripDisplay) ? 1u : 0u;
	store.checksum = App_Vehicle_CalcMileageChecksum(&store);

	(void)DRV_EEPROM_WriteBuffer(APP_VEHICLE_MILEAGE_EEPROM_ADDR, (const uint8_t *)&store,
								 sizeof(store));
}

static void App_Vehicle_LoadMileage(void)
{
	stc_app_vehicle_mileage_store_t store;

	if (TRUE == s_bVehicleMileageLoaded)
	{
		return;
	}

	(void)DRV_EEPROM_Init();
	if ((0 == DRV_EEPROM_ReadBuffer(APP_VEHICLE_MILEAGE_EEPROM_ADDR, (uint8_t *)&store,
									sizeof(store))) &&
		(TRUE == App_Vehicle_IsMileageStoreValid(&store)))
	{
		s_u32VehicleTotalTenthsKm = store.total_tenths_km;
		s_u16VehicleTripTenthsKm = store.trip_tenths_km;
		s_bVehicleUnitImperial = (store.unit_imperial != 0u) ? TRUE : FALSE;
		s_bVehicleMileageTripDisplay = (store.display_trip != 0u) ? TRUE : FALSE;
	}
	else
	{
		s_u32VehicleTotalTenthsKm = 0u;
		s_u16VehicleTripTenthsKm = 0u;
		s_bVehicleUnitImperial = FALSE;
		s_bVehicleMileageTripDisplay = FALSE;
		App_Vehicle_SaveMileage();
	}

	s_u32VehicleMileagePulseRemainder = 0u;
	s_u32VehicleMileageLastPulseCount = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdSpeed);
	s_bVehicleMileageLoaded = TRUE;
}

static void App_Vehicle_UpdateMileage(void)
{
	uint32_t current_pulse_count;
	uint32_t delta_pulses;
	uint32_t add_tenths;

	App_Vehicle_LoadMileage();

	current_pulse_count = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdSpeed);
	delta_pulses = current_pulse_count - s_u32VehicleMileageLastPulseCount;
	s_u32VehicleMileageLastPulseCount = current_pulse_count;

	if (0u == delta_pulses)
	{
		return;
	}

	s_u32VehicleMileagePulseRemainder += delta_pulses;
	add_tenths =
		s_u32VehicleMileagePulseRemainder / APP_VEHICLE_ODOMETER_PULSES_PER_TENTH;
	s_u32VehicleMileagePulseRemainder %= APP_VEHICLE_ODOMETER_PULSES_PER_TENTH;

	if (0u == add_tenths)
	{
		return;
	}

	if (s_u32VehicleTotalTenthsKm < APP_VEHICLE_TOTAL_MAX_TENTHS)
	{
		if (add_tenths > (APP_VEHICLE_TOTAL_MAX_TENTHS - s_u32VehicleTotalTenthsKm))
		{
			s_u32VehicleTotalTenthsKm = APP_VEHICLE_TOTAL_MAX_TENTHS;
		}
		else
		{
			s_u32VehicleTotalTenthsKm += add_tenths;
		}
	}

	s_u16VehicleTripTenthsKm = (uint16_t)((s_u16VehicleTripTenthsKm + add_tenths) %
										  (APP_VEHICLE_TRIP_MAX_TENTHS + 1u));
	App_Vehicle_SaveMileage();
}

#if (APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO != 0u)
static void App_Vehicle_UpdateTestFreqX100(void)
{
	uint32_t speed_pulse_count;
	uint32_t rpm_pulse_count;
	uint32_t speed_delta;
	uint32_t rpm_delta;
	uint32_t display_delta;
	uint32_t now_ms;
	uint32_t elapsed_ms;

	now_ms = BSP_SYS_GetTickMs();
	if (FALSE == s_bVehicleTestGateInited)
	{
		s_u32VehicleTestLastSpeedPulseCount =
			DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdSpeed);
		s_u32VehicleTestLastRpmPulseCount = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdRpm);
		s_u32VehicleTestGateStartMs = now_ms;
		s_bVehicleTestGateInited = TRUE;
		return;
	}

	elapsed_ms = now_ms - s_u32VehicleTestGateStartMs;
	if (elapsed_ms < 1000u)
	{
		return;
	}

	speed_pulse_count = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdSpeed);
	rpm_pulse_count = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdRpm);
	speed_delta = speed_pulse_count - s_u32VehicleTestLastSpeedPulseCount;
	rpm_delta = rpm_pulse_count - s_u32VehicleTestLastRpmPulseCount;
	s_u32VehicleTestLastSpeedPulseCount = speed_pulse_count;
	s_u32VehicleTestLastRpmPulseCount = rpm_pulse_count;
	s_u32VehicleTestGateStartMs = now_ms;

	display_delta = (speed_delta >= rpm_delta) ? speed_delta : rpm_delta;
	s_u32VehicleTestFreqX100 =
		(uint32_t)((((uint64_t)display_delta * 100000u) + (elapsed_ms / 2u)) /
				   elapsed_ms);
	if (s_u32VehicleTestFreqX100 > 999999u)
	{
		s_u32VehicleTestFreqX100 = 999999u;
	}
}
#endif

static void App_Vehicle_ShowMileage(void)
{
#if (APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO != 0u)
	LedPanel_Set(LedPanelIdOdo, TRUE);
	(void)LedPanel_ShowTotalOdometer(s_u32VehicleTestFreqX100);
#else
	LedPanel_Set(s_bVehicleMileageTripDisplay ? LedPanelIdTrip : LedPanelIdOdo, TRUE);

	if (TRUE == s_bVehicleMileageTripDisplay)
	{
		uint16_t display_val = s_u16VehicleTripTenthsKm;
		if (s_bVehicleUnitImperial)
		{
			display_val = (uint16_t)(((uint32_t)display_val * 1000u) / 1609u);
		}
		(void)LedPanel_ShowTripOdometer(display_val);
	}
	else
	{
		uint32_t display_val = s_u32VehicleTotalTenthsKm / 10u;
		if (s_bVehicleUnitImperial)
		{
			display_val = (display_val * 1000u) / 1609u;
		}
		(void)LedPanel_ShowTotalOdometer(display_val);
	}
#endif
}

static void App_Vehicle_UpdateClockCache(void)
{
	stc_rtc_time_t rtc_time;

	if (s_bVehicleClockSettingMode)
	{
		return;
	}

	if (Ok != DRV_RTC_ReadDateTime(&rtc_time))
	{
		return;
	}

	s_u8VehicleClockHour = (uint8_t)BCD2DEC(rtc_time.u8Hour);
	s_u8VehicleClockMinute = (uint8_t)BCD2DEC(rtc_time.u8Minute);
	s_bVehicleClockValid = TRUE;
}

static void App_Vehicle_ShowClock(void)
{
	if (FALSE == s_bVehicleClockValid)
	{
		App_Vehicle_UpdateClockCache();
	}

	LedPanel_Set(LedPanelIdClockColon, TRUE);
	if (TRUE == s_bVehicleClockValid)
	{
		uint8_t show_hour = s_u8VehicleClockHour;
		uint8_t show_minute = s_u8VehicleClockMinute;

		if (s_bVehicleClockSettingMode && !s_bVehicleClockBlinkState)
		{
			if (s_bVehicleClockSettingHour)
			{
				show_hour = 0xFFu;
			}
			else
			{
				show_minute = 0xFFu;
			}
		}

		(void)LedPanel_ShowClock(show_hour, show_minute);
	}
}

static uint16_t App_Vehicle_GetCurrentSpeed(void)
{
	uint32_t freq_mhz;
	uint32_t denominator;
	uint32_t speed;

	if (FALSE ==
		DEV_SpeedRpm_IsValidByMeasure(DevSpeedRpmIdSpeed, APP_VEHICLE_FREQ_MEASURE))
	{
		s_u16VehicleDisplaySpeed = 0u;
		s_u16VehicleSpeedCandidate = 0u;
		s_u8VehicleSpeedCandidateTicks = 0u;
		s_bVehicleSpeedDisplayInited = FALSE;
		return 0u;
	}

	freq_mhz = DEV_SpeedRpm_GetFreqMilliHzByMeasure(DevSpeedRpmIdSpeed,
													APP_VEHICLE_FREQ_MEASURE);
	denominator = APP_VEHICLE_SPEED_PULSES_PER_KM * APP_VEHICLE_MILLIHZ_PER_HZ;
	speed = ((uint64_t)freq_mhz * 3600u + (denominator / 2u)) / denominator;

	if (speed > 199u)
	{
		speed = 199u;
	}
	speed = App_Vehicle_MapSpeed((uint16_t)speed);
	if (speed > 199u)
	{
		speed = 199u;
	}
	if (0u == speed)
	{
		s_u16VehicleDisplaySpeed = 0u;
		s_u16VehicleSpeedCandidate = 0u;
		s_u8VehicleSpeedCandidateTicks = 0u;
		s_bVehicleSpeedDisplayInited = FALSE;
		return 0u;
	}

	return App_Vehicle_ConfirmDisplayU16(
		(uint16_t)speed, &s_u16VehicleDisplaySpeed, &s_u16VehicleSpeedCandidate,
		&s_u8VehicleSpeedCandidateTicks, &s_bVehicleSpeedDisplayInited);
}

static uint32_t App_Vehicle_GetCurrentEngineRpm(void)
{
	uint32_t freq_mhz;
	uint32_t denominator;

	if (FALSE ==
		DEV_SpeedRpm_IsValidByMeasure(DevSpeedRpmIdRpm, APP_VEHICLE_FREQ_MEASURE))
	{
		return 0u;
	}

	freq_mhz =
		DEV_SpeedRpm_GetFreqMilliHzByMeasure(DevSpeedRpmIdRpm, APP_VEHICLE_FREQ_MEASURE);
	denominator = APP_VEHICLE_MILLIHZ_PER_HZ * APP_VEHICLE_RPM_RATIO_DENOMINATOR *
				  APP_VEHICLE_HALL_POLE_PAIRS;

	return (uint32_t)(((uint64_t)freq_mhz * 60u * APP_VEHICLE_RPM_RATIO_NUMERATOR +
					   (denominator / 2u)) /
					  denominator);
}

static uint8_t App_Vehicle_GetCurrentRpmBarCount(void)
{
	uint32_t rpm;
	uint32_t bar_count;

	rpm = App_Vehicle_GetCurrentEngineRpm();
	bar_count = (rpm + APP_VEHICLE_RPM_PER_BAR - 1u) / APP_VEHICLE_RPM_PER_BAR;
	if (bar_count > LED_PANEL_RPM_BAR_COUNT)
	{
		bar_count = LED_PANEL_RPM_BAR_COUNT;
	}

	return App_Vehicle_ConfirmDisplayU8(
		(uint8_t)bar_count, &s_u8VehicleDisplayRpmBars, &s_u8VehicleRpmBarsCandidate,
		&s_u8VehicleRpmBarsCandidateTicks, &s_bVehicleRpmBarsDisplayInited);
}

static uint8_t App_Vehicle_GetCurrentGear(void)
{
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear1))
	{
		return 1u;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear2))
	{
		return 2u;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear3))
	{
		return 3u;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear4))
	{
		return 4u;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear5))
	{
		return 5u;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear6))
	{
		return 6u;
	}

	return 0u;
}

static uint8_t App_Vehicle_GetActiveGearCount(void)
{
	uint8_t count;

	count = 0u;
	if (TRUE == DRV_Input_IsActive(DrvInputIdGearN))
	{
		count++;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear1))
	{
		count++;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear2))
	{
		count++;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear3))
	{
		count++;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear4))
	{
		count++;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear5))
	{
		count++;
	}
	if (TRUE == DRV_Input_IsActive(DrvInputIdGear6))
	{
		count++;
	}

	return count;
}

static void App_Vehicle_UpdateGearBlink(void)
{
	s_u8VehicleGearBlinkTick++;
	if (s_u8VehicleGearBlinkTick >= APP_VEHICLE_GEAR_BLINK_TICKS)
	{
		s_u8VehicleGearBlinkTick = 0u;
		s_bVehicleGearBlinkOn = (TRUE == s_bVehicleGearBlinkOn) ? FALSE : TRUE;
	}
}

static void App_Vehicle_ShowCurrentGear(void)
{
	uint8_t gear;
	uint8_t active_count;
	boolean_t gear_on;

	gear = App_Vehicle_GetCurrentGear();
	active_count = App_Vehicle_GetActiveGearCount();
	gear_on = (active_count > 1u) ? s_bVehicleGearBlinkOn : TRUE;

	if (TRUE == gear_on)
	{
		LedPanel_ShowGearDigit(gear);
	}
	else
	{
		LedPanel_ClearGearDigit();
	}

	LedPanel_Set(LedPanelIdGearN,
				 (boolean_t)((TRUE == DRV_Input_IsActive(DrvInputIdGearN)) &&
							 ((active_count <= 1u) || (TRUE == gear_on))));
}

static uint8_t App_Vehicle_GetFuelTargetBars(void)
{
	uint16_t resistance_ohm;

	resistance_ohm = DRV_ADC_GetResistanceOhm(BspAdcIdFuel);
	if (DRV_ADC_RESISTANCE_INVALID_OHM == resistance_ohm)
	{
		return APP_VEHICLE_FUEL_INVALID_BARS;
	}
	resistance_ohm =
		(uint16_t)(((uint32_t)resistance_ohm * APP_VEHICLE_FUEL_RES_CORRECT_NUMERATOR +
					(APP_VEHICLE_FUEL_RES_CORRECT_DENOMINATOR / 2u)) /
				   APP_VEHICLE_FUEL_RES_CORRECT_DENOMINATOR);

	if (resistance_ohm > 76u)
	{
		return 1u;
	}
	if (resistance_ohm > 66u)
	{
		return 2u;
	}
	if (resistance_ohm > 54u)
	{
		return 3u;
	}
	if (resistance_ohm > 43u)
	{
		return 4u;
	}
	if (resistance_ohm > 31u)
	{
		return 5u;
	}
	if (resistance_ohm > 18u)
	{
		return 6u;
	}
	if (resistance_ohm > 10u)
	{
		return 7u;
	}

	return 8u;
}

static uint8_t App_Vehicle_GetCurrentFuelBars(void)
{
	uint8_t target_bars;

	target_bars = App_Vehicle_GetFuelTargetBars();
	if ((FALSE == s_bVehicleFuelInited) ||
		(s_u16VehicleFuelFastTick < APP_VEHICLE_FUEL_FAST_TICKS))
	{
		s_bVehicleFuelInited = TRUE;
		s_u8VehicleFuelDisplayBars = target_bars;
		if (s_u16VehicleFuelFastTick < APP_VEHICLE_FUEL_FAST_TICKS)
		{
			s_u16VehicleFuelFastTick++;
		}
		return s_u8VehicleFuelDisplayBars;
	}

	if (s_u8VehicleFuelDisplayBars == target_bars)
	{
		s_u16VehicleFuelSlowTick = 0u;
		return s_u8VehicleFuelDisplayBars;
	}

	if (s_u16VehicleFuelSlowTick < APP_VEHICLE_FUEL_SLOW_TICKS)
	{
		s_u16VehicleFuelSlowTick++;
		return s_u8VehicleFuelDisplayBars;
	}

	s_u16VehicleFuelSlowTick = 0u;
	if (s_u8VehicleFuelDisplayBars < target_bars)
	{
		s_u8VehicleFuelDisplayBars++;
	}
	else
	{
		s_u8VehicleFuelDisplayBars--;
	}

	return s_u8VehicleFuelDisplayBars;
}

static void App_Vehicle_ShowFuel(void)
{
	uint8_t fuel_bars;
	boolean_t low_fuel_blink_on;

	fuel_bars = App_Vehicle_GetCurrentFuelBars();
	low_fuel_blink_on = (boolean_t)((fuel_bars != 1u) || (TRUE == s_bVehicleGearBlinkOn));

	LedPanel_Set(LedPanelIdFuelWhite, FALSE);
	LedPanel_Set(LedPanelIdFuelYellow, TRUE);
	LedPanel_ShowFuelBars(low_fuel_blink_on ? fuel_bars : 0u);
}

static void App_Vehicle_ShowIndicators(void)
{
	LedPanel_Set(LedPanelIdLeftTurn, DRV_Input_IsActive(DrvInputIdLeftTurn));
	LedPanel_Set(LedPanelIdRightTurn, DRV_Input_IsActive(DrvInputIdRightTurn));
	LedPanel_Set(LedPanelIdHighBeam, DRV_Input_IsActive(DrvInputIdHighBeam));
	LedPanel_Set(LedPanelIdEngineFault, DRV_Input_IsActive(DrvInputIdEnginefault));
	LedPanel_Set(LedPanelIdBatteryFault, FALSE);
}

static void App_Vehicle_ShowSelfCheckFrame(uint16_t tick)
{
	uint8_t digit;
	uint8_t rpm_bar_count;
	uint8_t fuel_bar_count;
	uint32_t odo_value;
	uint16_t phase_tick;

	phase_tick = tick;
	if (phase_tick < APP_VEHICLE_SELF_CHECK_SETUP_TICKS)
	{
		LedPanel_Clear();
		LedPanel_SetBorder(TRUE);
		LedPanel_ShowRpmBars(LED_PANEL_RPM_BAR_COUNT);
		App_Vehicle_ShowSelfCheckSpeedRaw(0u, 0u, FALSE);
		LedPanel_Refresh();
		return;
	}

	phase_tick = (uint16_t)(phase_tick - APP_VEHICLE_SELF_CHECK_SETUP_TICKS);
	if (phase_tick < APP_VEHICLE_SELF_CHECK_RPM_DOWN_TICKS)
	{
		rpm_bar_count =
			(uint8_t)(LED_PANEL_RPM_BAR_COUNT -
					  (((uint32_t)phase_tick * (LED_PANEL_RPM_BAR_COUNT - 1u)) /
					   (APP_VEHICLE_SELF_CHECK_RPM_DOWN_TICKS - 1u)));

		LedPanel_Clear();
		LedPanel_SetBorder(TRUE);
		LedPanel_ShowRpmBars(rpm_bar_count);
		if (phase_tick < (9u * APP_VEHICLE_SELF_CHECK_SPEED_DIGIT_TICKS))
		{
			digit =
				(uint8_t)((phase_tick / APP_VEHICLE_SELF_CHECK_SPEED_DIGIT_TICKS) + 1u);
			App_Vehicle_ShowSelfCheckSpeedRaw(digit, digit, FALSE);
		}
		else
		{
			App_Vehicle_ShowSelfCheckSpeedSweep(
				(uint16_t)(phase_tick - (9u * APP_VEHICLE_SELF_CHECK_SPEED_DIGIT_TICKS)));
		}
		LedPanel_Refresh();
		return;
	}

	phase_tick = (uint16_t)(phase_tick - APP_VEHICLE_SELF_CHECK_RPM_DOWN_TICKS);
	if (phase_tick < APP_VEHICLE_SELF_CHECK_FULL_ON_TICKS)
	{
		LedPanel_Fill();
		LedPanel_ShowClock(0u, 0u);
		LedPanel_ShowGearDigit(0u);
		App_Vehicle_ShowSelfCheckSpeedRaw(0u, 0u, TRUE);
		LedPanel_ShowOdometer(0u, 0u);
		LedPanel_ShowRpmBars(1u);
		LedPanel_ShowFuelBars(0u);
		LedPanel_Refresh();
		return;
	}

	phase_tick = (uint16_t)(phase_tick - APP_VEHICLE_SELF_CHECK_FULL_ON_TICKS);
	if (phase_tick >= APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS)
	{
		phase_tick = (uint16_t)(APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS - 1u);
	}

	LedPanel_Fill();
	digit =
		(uint8_t)(((uint32_t)phase_tick * 10u) / APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS);
	if (digit > 9u)
	{
		digit = 9u;
	}
	rpm_bar_count =
		(uint8_t)(1u + (((uint32_t)phase_tick * (LED_PANEL_RPM_BAR_COUNT - 1u)) /
						(APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS - 1u)));
	fuel_bar_count = (uint8_t)(((uint32_t)phase_tick * LED_PANEL_FUEL_BAR_COUNT) /
							   (APP_VEHICLE_SELF_CHECK_RAMP_UP_TICKS - 1u));
	odo_value = (uint32_t)digit * 11111u;

	LedPanel_ShowClock((uint8_t)(digit * 11u), (uint8_t)(digit * 11u));
	LedPanel_ShowGearDigit(digit);
	App_Vehicle_ShowSelfCheckSpeedRaw(digit, digit, TRUE);
	LedPanel_ShowOdometer(odo_value, digit);
	LedPanel_ShowRpmBars(rpm_bar_count);
	LedPanel_ShowFuelBars(fuel_bar_count);
	LedPanel_Refresh();
}

static void App_Vehicle_ShowNormalFrame(void)
{
	App_Vehicle_UpdateGearBlink();
#if (APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO == 0u)
	App_Vehicle_UpdateMileage();
#else
	App_Vehicle_UpdateTestFreqX100();
#endif

	LedPanel_Clear();
	LedPanel_SetBorder(TRUE);
	App_Vehicle_ShowIndicators();
	App_Vehicle_ShowClock();

	if (s_bVehicleUnitImperial)
	{
		LedPanel_Set(LedPanelIdSpeedMph, TRUE);
		LedPanel_Set(LedPanelIdMileageMile, TRUE);
		uint16_t speed_km = App_Vehicle_GetCurrentSpeed();
		LedPanel_ShowSpeed((uint16_t)(((uint32_t)speed_km * 1000u) / 1609u));
	}
	else
	{
		LedPanel_Set(LedPanelIdSpeedKm, TRUE);
		LedPanel_Set(LedPanelIdMileageKm, TRUE);
		LedPanel_ShowSpeed(App_Vehicle_GetCurrentSpeed());
	}

	App_Vehicle_ShowMileage();
	App_Vehicle_ShowCurrentGear();
	LedPanel_ShowRpmBars(App_Vehicle_GetCurrentRpmBarCount());
	App_Vehicle_ShowFuel();
	LedPanel_Refresh();
}

void App_Vehicle_ResetSelfCheck(void)
{
	s_bVehicleSelfCheckStarted = FALSE;
	s_u16VehicleSelfCheckTick = 0u;
	s_u8VehicleNormalRefreshTick = 0u;
	s_bVehicleFuelInited = FALSE;
	s_u16VehicleFuelFastTick = 0u;
	s_u16VehicleFuelSlowTick = 0u;
	s_u8VehicleGearBlinkTick = 0u;
	s_bVehicleGearBlinkOn = TRUE;
	s_u16VehicleDisplaySpeed = 0u;
	s_u16VehicleSpeedCandidate = 0u;
	s_u8VehicleSpeedCandidateTicks = 0u;
	s_bVehicleSpeedDisplayInited = FALSE;
	s_u8VehicleDisplayRpmBars = 0u;
	s_u8VehicleRpmBarsCandidate = 0u;
	s_u8VehicleRpmBarsCandidateTicks = 0u;
	s_bVehicleRpmBarsDisplayInited = FALSE;
#if (APP_VEHICLE_TEST_SHOW_FREQ_X100_ON_ODO != 0u)
	s_u32VehicleTestLastSpeedPulseCount = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdSpeed);
	s_u32VehicleTestLastRpmPulseCount = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdRpm);
	s_u32VehicleTestGateStartMs = BSP_SYS_GetTickMs();
	s_u32VehicleTestFreqX100 = 0u;
	s_bVehicleTestGateInited = FALSE;
#endif
	s_bVehicleClockValid = FALSE;
	App_Vehicle_LoadMileage();
	App_Vehicle_UpdateClockCache();
	s_u32VehicleMileageLastPulseCount = DEV_SpeedRpm_GetPulseCount(DevSpeedRpmIdSpeed);
}

boolean_t App_Vehicle_SelfCheckTask10ms(void)
{
	if (FALSE == s_bVehicleSelfCheckStarted)
	{
		App_Vehicle_ShowSelfCheckFrame(s_u16VehicleSelfCheckTick);
		s_u16VehicleSelfCheckTick++;

		if (s_u16VehicleSelfCheckTick >= APP_VEHICLE_SELF_CHECK_TICKS)
		{
			s_bVehicleSelfCheckStarted = TRUE;
			App_Vehicle_ShowNormalFrame();
		}
	}

	return s_bVehicleSelfCheckStarted;

	// 卡在全亮
	// Bsp_Tm3100Led_Fill();
	// Bsp_Tm3100Led_Refresh();
	// return FALSE;
}

void App_Vehicle_NotifyRtcTick500ms(void)
{
	App_Vehicle_UpdateClockCache();
}

static void App_Vehicle_ProcessButtons(void)
{
	en_drv_button_event_t eventK1 = DRV_Button_GetEvent(DrvButtonIdK1);
	en_drv_button_event_t eventK2 = DRV_Button_GetEvent(DrvButtonIdK2);
	en_drv_button_event_t eventBoth = DRV_Button_GetEvent(DrvButtonIdBoth);

	if (s_bVehicleClockSettingMode)
	{
		if (DRV_Button_IsTimeout10s())
		{
			stc_rtc_time_t rtc_time;

			s_bVehicleClockSettingMode = FALSE;

			if (Ok == DRV_RTC_ReadDateTime(&rtc_time))
			{
				rtc_time.u8Hour = (uint8_t)((((s_u8VehicleClockHour) / 10u) << 4u) |
											((s_u8VehicleClockHour) % 10u));
				rtc_time.u8Minute = (uint8_t)((((s_u8VehicleClockMinute) / 10u) << 4u) |
											  ((s_u8VehicleClockMinute) % 10u));
				(void)DRV_RTC_SetTime(&rtc_time);
			}
		}
		else
		{
			s_u8VehicleClockBlinkTick++;
			if (s_u8VehicleClockBlinkTick >= 5u)
			{
				s_u8VehicleClockBlinkTick = 0u;
				s_bVehicleClockBlinkState =
					(TRUE == s_bVehicleClockBlinkState) ? FALSE : TRUE;
			}
		}
	}

	if (eventBoth == DrvButtonEventLongPress)
	{
		s_bVehicleClockSettingMode = TRUE;
		s_bVehicleClockSettingHour = TRUE;
		s_bVehicleClockBlinkState = TRUE;
		s_u8VehicleClockBlinkTick = 0u;
		DRV_Button_ClearTimeout();
	}

	if (s_bVehicleClockSettingMode)
	{
		if (eventK1 == DrvButtonEventShortPress)
		{
			if (s_bVehicleClockSettingHour)
			{
				s_u8VehicleClockHour = (uint8_t)((s_u8VehicleClockHour + 1u) % 24u);
			}
			else
			{
				s_u8VehicleClockMinute = (uint8_t)((s_u8VehicleClockMinute + 1u) % 60u);
			}
			DRV_Button_ClearTimeout();
			s_bVehicleClockBlinkState = TRUE;
		}
		else if (eventK2 == DrvButtonEventShortPress)
		{
			s_bVehicleClockSettingHour =
				(TRUE == s_bVehicleClockSettingHour) ? FALSE : TRUE;
			DRV_Button_ClearTimeout();
			s_bVehicleClockBlinkState = TRUE;
		}
	}
	else
	{
		if (eventK1 == DrvButtonEventShortPress)
		{
			s_bVehicleMileageTripDisplay =
				(TRUE == s_bVehicleMileageTripDisplay) ? FALSE : TRUE;
			App_Vehicle_SaveMileage();
		}
		else if (eventK2 == DrvButtonEventLongPress)
		{
			if (s_bVehicleMileageTripDisplay)
			{
				s_u16VehicleTripTenthsKm = 0u;
				App_Vehicle_SaveMileage();
			}
			else
			{
				s_bVehicleUnitImperial = (TRUE == s_bVehicleUnitImperial) ? FALSE : TRUE;
				App_Vehicle_SaveMileage();
			}
		}
	}
}

void App_Vehicle_Task10ms(void)
{
	s_u8VehicleNormalRefreshTick++;
	if (s_u8VehicleNormalRefreshTick < APP_VEHICLE_NORMAL_REFRESH_TICKS)
	{
		return;
	}

	s_u8VehicleNormalRefreshTick = 0u;

	App_Vehicle_ProcessButtons();
	App_Vehicle_ShowNormalFrame();
}
