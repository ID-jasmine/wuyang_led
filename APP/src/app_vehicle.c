#include "app_vehicle.h"

#include "bsp_tm3100_led.h"
#include "dev_speed_rpm.h"
#include "drv_adc.h"
#include "drv_button.h"
#include "drv_eeprom.h"
#include "drv_input.h"
#include "drv_rtc.h"
#include "led_panel.h"

#define APP_VEHICLE_SELF_CHECK_TICKS		  (150u)
#define APP_VEHICLE_SELF_CHECK_STEP_COUNT	  (19u)
#define APP_VEHICLE_NORMAL_REFRESH_TICKS	  (10u)
#define APP_VEHICLE_ADC_FULL_SCALE			  (4095u)
#define APP_VEHICLE_SPEED_PULSES_PER_KM		  (2850u)
#define APP_VEHICLE_MILLIHZ_PER_HZ			  (1000u)
#define APP_VEHICLE_RPM_RATIO_NUMERATOR		  (667u)
#define APP_VEHICLE_RPM_RATIO_DENOMINATOR	  (100u)
#define APP_VEHICLE_RPM_PER_BAR				  (500u)
#define APP_VEHICLE_GEAR_BLINK_TICKS		  (5u)
#define APP_VEHICLE_FUEL_FAST_TICKS			  (30u)
#define APP_VEHICLE_FUEL_SLOW_TICKS			  (300u)
#define APP_VEHICLE_FUEL_INVALID_BARS		  (0u)
#define APP_VEHICLE_ODOMETER_PULSES_PER_KM	  (2800u)
#define APP_VEHICLE_ODOMETER_PULSES_PER_TENTH (APP_VEHICLE_ODOMETER_PULSES_PER_KM / 10u)
#define APP_VEHICLE_TRIP_MAX_TENTHS			  (9999u)
#define APP_VEHICLE_TOTAL_MAX_TENTHS		  (9999990u)
#define APP_VEHICLE_MILEAGE_EEPROM_ADDR		  (0u)
#define APP_VEHICLE_MILEAGE_MAGIC			  (0x4F444F31u)
#define APP_VEHICLE_MILEAGE_VERSION			  (1u)

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

/* Clock state */
static uint8_t s_u8VehicleClockHour = 0u;
static uint8_t s_u8VehicleClockMinute = 0u;
static boolean_t s_bVehicleClockValid = FALSE;
static boolean_t s_bVehicleClockSettingMode = FALSE;
static boolean_t s_bVehicleClockSettingHour = TRUE;
static boolean_t s_bVehicleClockBlinkState = FALSE;
static uint8_t s_u8VehicleClockBlinkTick = 0;

static uint8_t App_Vehicle_GetSelfCheckStep(uint16_t tick)
{
	if (tick >= APP_VEHICLE_SELF_CHECK_TICKS)
	{
		tick = (uint16_t)(APP_VEHICLE_SELF_CHECK_TICKS - 1u);
	}

	return (uint8_t)((APP_VEHICLE_SELF_CHECK_STEP_COUNT - 1u) -
					 ((tick * APP_VEHICLE_SELF_CHECK_STEP_COUNT) /
					  APP_VEHICLE_SELF_CHECK_TICKS));
}

static uint8_t App_Vehicle_GetSelfCheckDigit(uint8_t step)
{
	if (step >= 10u)
	{
		return (uint8_t)(step - 9u);
	}

	return step;
}

static uint16_t App_Vehicle_GetSelfCheckSpeed(uint8_t step)
{
	if (step >= 9u)
	{
		return (uint16_t)(100u + ((uint16_t)(step - 9u) * 11u));
	}

	return (uint16_t)((step + 1u) * 11u);
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

static void App_Vehicle_ShowMileage(void)
{
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

	if (FALSE == DEV_SpeedRpm_IsValid(DevSpeedRpmIdSpeed))
	{
		return 0u;
	}

	freq_mhz = DEV_SpeedRpm_GetFreqMilliHz(DevSpeedRpmIdSpeed);
	denominator = APP_VEHICLE_SPEED_PULSES_PER_KM * APP_VEHICLE_MILLIHZ_PER_HZ;
	speed = ((uint64_t)freq_mhz * 3600u + (denominator / 2u)) / denominator;

	if (speed > 199u)
	{
		speed = 199u;
	}

	return (uint16_t)speed;
}

static uint32_t App_Vehicle_GetCurrentEngineRpm(void)
{
	uint32_t freq_mhz;
	uint32_t denominator;

	if (FALSE == DEV_SpeedRpm_IsValid(DevSpeedRpmIdRpm))
	{
		return 0u;
	}

	freq_mhz = DEV_SpeedRpm_GetFreqMilliHz(DevSpeedRpmIdRpm);
	denominator = APP_VEHICLE_MILLIHZ_PER_HZ * APP_VEHICLE_RPM_RATIO_DENOMINATOR;

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

	return (uint8_t)bar_count;
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
	uint8_t step;
	uint8_t progress;
	uint8_t rpm_bar_count;
	uint8_t fuel_bar_count;
	uint32_t odo_value;

	step = App_Vehicle_GetSelfCheckStep(tick);
	digit = App_Vehicle_GetSelfCheckDigit(step);
	progress = (uint8_t)(((uint32_t)(tick + 1u) * 100u) / APP_VEHICLE_SELF_CHECK_TICKS);
	rpm_bar_count =
		(uint8_t)(((uint16_t)progress * LED_PANEL_RPM_BAR_COUNT + 99u) / 100u);
	fuel_bar_count =
		(uint8_t)(((uint16_t)progress * LED_PANEL_FUEL_BAR_COUNT + 99u) / 100u);
	odo_value = (uint32_t)digit * 11111u;

	LedPanel_Fill();
	LedPanel_ShowClock((uint8_t)(digit * 11u), (uint8_t)(digit * 11u));
	LedPanel_ShowGearDigit(digit);
	LedPanel_ShowSpeed(App_Vehicle_GetSelfCheckSpeed(step));
	LedPanel_ShowOdometer(odo_value, digit);
	LedPanel_ShowRpmBars(rpm_bar_count);
	LedPanel_ShowFuelBars(fuel_bar_count);
	LedPanel_Refresh();
}

static void App_Vehicle_ShowNormalFrame(void)
{
	App_Vehicle_UpdateGearBlink();
	App_Vehicle_UpdateMileage();

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

void App_Vehicle_NotifyRtcTick1s(void)
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
