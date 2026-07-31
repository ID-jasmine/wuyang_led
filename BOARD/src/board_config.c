#include "board_config.h"

#define BOARD_PIN_GPIO(port_, pin_, dir_, pu_, pd_, od_, init_, sleep_)                  \
	{TRUE, port_, pin_, BoardPinModeGpio, sleep_, dir_, pu_, pd_, od_, init_, GpioAf0}

#define BOARD_PIN_ANALOG(port_, pin_)                                                     \
	{TRUE, port_, pin_, BoardPinModeAnalog, BoardPinSleepKeep, GpioDirIn,                  \
	 GpioPuDisable, GpioPdDisable, GpioOdDisable, FALSE, GpioAf0}

#define BOARD_PIN_PERIPHERAL(enabled_, port_, pin_, af_, sleep_)                          \
	{enabled_, port_, pin_, BoardPinModePeripheral, sleep_, GpioDirIn,                    \
	 GpioPuDisable, GpioPdDisable, GpioOdDisable, FALSE, af_}

const stc_board_pin_cfg_t g_astBoardPinCfg[BoardPinIdCount] = {
	[BoardPinIdPower] = BOARD_PIN_GPIO(GpioPortB, GpioPin3, GpioDirOut, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepDefaultOutput),
	[BoardPinIdPositionLamp] = BOARD_PIN_GPIO(GpioPortA, GpioPin3, GpioDirIn,
										 GpioPuDisable, GpioPdDisable, GpioOdDisable,
										 FALSE, BoardPinSleepAnalog),
	[BoardPinIdSwK1] = BOARD_PIN_GPIO(GpioPortD, GpioPin6, GpioDirIn, GpioPuEnable,
									 GpioPdDisable, GpioOdDisable, FALSE,
									 BoardPinSleepAnalog),
	[BoardPinIdSwK2] = BOARD_PIN_GPIO(GpioPortD, GpioPin7, GpioDirIn, GpioPuEnable,
									 GpioPdDisable, GpioOdDisable, FALSE,
									 BoardPinSleepAnalog),
	[BoardPinIdGearN] = BOARD_PIN_GPIO(GpioPortB, GpioPin1, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdGear1] = BOARD_PIN_GPIO(GpioPortD, GpioPin1, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdGear2] = BOARD_PIN_GPIO(GpioPortA, GpioPin7, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdGear3] = BOARD_PIN_GPIO(GpioPortB, GpioPin0, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdGear4] = BOARD_PIN_GPIO(GpioPortB, GpioPin2, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdGear5] = BOARD_PIN_GPIO(GpioPortC, GpioPin13, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdGear6] = BOARD_PIN_GPIO(GpioPortD, GpioPin0, GpioDirIn, GpioPuDisable,
									  GpioPdDisable, GpioOdDisable, FALSE,
									  BoardPinSleepAnalog),
	[BoardPinIdEngineFault] = BOARD_PIN_GPIO(GpioPortB, GpioPin10, GpioDirIn,
										GpioPuDisable, GpioPdDisable, GpioOdDisable,
										FALSE, BoardPinSleepAnalog),
	[BoardPinIdLedPower] = BOARD_PIN_GPIO(GpioPortB, GpioPin9, GpioDirOut,
										GpioPuDisable, GpioPdDisable, GpioOdDisable,
										FALSE, BoardPinSleepDefaultOutput),

	[BoardPinIdEepromScl] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_EEPROM, GpioPortB,
											 GpioPin6, GpioAf0, BoardPinSleepAnalog),
	[BoardPinIdEepromSda] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_EEPROM, GpioPortB,
											 GpioPin7, GpioAf0, BoardPinSleepAnalog),

	[BoardPinIdNcPb8] = BOARD_PIN_ANALOG(GpioPortB, GpioPin8),
	[BoardPinIdNcPb5] = BOARD_PIN_ANALOG(GpioPortB, GpioPin5),
	[BoardPinIdNcPb4] = BOARD_PIN_ANALOG(GpioPortB, GpioPin4),
	[BoardPinIdNcPa15] = BOARD_PIN_ANALOG(GpioPortA, GpioPin15),
	[BoardPinIdNcPa12] = BOARD_PIN_ANALOG(GpioPortA, GpioPin12),
	[BoardPinIdNcPa11] = BOARD_PIN_ANALOG(GpioPortA, GpioPin11),

	[BoardPinIdAdcFuel] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortA,
										   GpioPin1, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcPower] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortA,
											GpioPin0, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcWaterTemp] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortA,
												GpioPin2, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcIgn] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortB,
										  GpioPin11, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcBrightness] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortB,
												 GpioPin12, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcLeftTurn] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortA,
											   GpioPin5, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcHighBeam] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortA,
											   GpioPin6, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdAdcRightTurn] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_ADC, GpioPortA,
												GpioPin4, GpioAf0, BoardPinSleepKeep),

	[BoardPinIdCaptureSpeed] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_SPEED_RPM,
												GpioPortA, GpioPin10, GpioAf2,
												BoardPinSleepAnalog),
	[BoardPinIdCaptureRpm] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_SPEED_RPM,
											  GpioPortA, GpioPin9, GpioAf2,
											  BoardPinSleepAnalog),

	[BoardPinIdTm3100Sdi] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_LED_PANEL, GpioPortA,
											  GpioPin8, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdTm3100Clk] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_LED_PANEL, GpioPortB,
											  GpioPin15, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdTm3100Le] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_LED_PANEL, GpioPortB,
											 GpioPin14, GpioAf0, BoardPinSleepKeep),
	[BoardPinIdTm3100Oe] = BOARD_PIN_PERIPHERAL(PRODUCT_FEATURE_LED_PANEL, GpioPortB,
											 GpioPin13, GpioAf5, BoardPinSleepKeep),
};

const stc_board_adc_cfg_t g_astBoardAdcCfg[BoardAdcIdCount] = {
	[BoardAdcIdFuel] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcFuel, AdcExInputCH1},
	[BoardAdcIdPower] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcPower, AdcExInputCH0},
	[BoardAdcIdWaterTemp] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcWaterTemp, AdcExInputCH2},
	[BoardAdcIdIgn] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcIgn, AdcExInputCH18},
	[BoardAdcIdBrightness] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcBrightness, AdcExInputCH19},
	[BoardAdcIdLeftTurn] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcLeftTurn, AdcExInputCH5},
	[BoardAdcIdHighBeam] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcHighBeam, AdcExInputCH6},
	[BoardAdcIdRightTurn] = {PRODUCT_FEATURE_ADC, BoardPinIdAdcRightTurn, AdcExInputCH4},
};

const stc_board_iic_cfg_t g_astBoardIicCfg[BoardIicBusIdCount] = {
	[BoardIicBusIdEeprom] = {PRODUCT_FEATURE_EEPROM, BoardPinIdEepromScl,
								   BoardPinIdEepromSda, 100000u},
};

const stc_board_capture_cfg_t g_astBoardCaptureCfg[BoardCaptureIdCount] = {
	[BoardCaptureIdSpeed] = {PRODUCT_FEATURE_SPEED_RPM, BoardPinIdCaptureSpeed,
								   BoardCaptureChannel2A},
	[BoardCaptureIdRpm] = {PRODUCT_FEATURE_SPEED_RPM, BoardPinIdCaptureRpm,
								 BoardCaptureChannel1A},
};

const stc_board_tm3100_cfg_t g_stcBoardTm3100Cfg = {
	PRODUCT_FEATURE_LED_PANEL,
	BoardPinIdTm3100Sdi,
	BoardPinIdTm3100Clk,
	BoardPinIdTm3100Le,
	BoardPinIdTm3100Oe,
	TIM1,
	1000u,
	BOARD_TM3100_CHIP_COUNT,
};

const stc_board_pin_cfg_t *Board_GetPinConfig(en_board_pin_id_t id)
{
	if (id >= BoardPinIdCount)
	{
		return NULL;
	}

	return &g_astBoardPinCfg[id];
}

static boolean_t Board_IsEnabledPeripheralPin(en_board_pin_id_t id)
{
	const stc_board_pin_cfg_t *pstcPinCfg;

	pstcPinCfg = Board_GetPinConfig(id);
	return (boolean_t)((NULL != pstcPinCfg) && (TRUE == pstcPinCfg->enabled) &&
						   (BoardPinModePeripheral == pstcPinCfg->run_mode));
}

en_result_t Board_ConfigValidate(void)
{
	uint8_t i;
	uint8_t j;
	uint8_t reference_count;

	/* 每个物理引脚只能在唯一引脚清单中出现一次。 */
	for (i = 0u; i < BoardPinIdCount; i++)
	{
		if (FALSE == g_astBoardPinCfg[i].enabled)
		{
			continue;
		}

		for (j = (uint8_t)(i + 1u); j < BoardPinIdCount; j++)
		{
			if ((TRUE == g_astBoardPinCfg[j].enabled) &&
				(g_astBoardPinCfg[i].port == g_astBoardPinCfg[j].port) &&
				(g_astBoardPinCfg[i].pin == g_astBoardPinCfg[j].pin))
			{
				return ErrorInvalidParameter;
			}
		}
	}

	/* 外设实例必须引用有效引脚；同一逻辑引脚只能归一个外设实例所有。 */
	for (i = 0u; i < BoardPinIdCount; i++)
	{
		if ((FALSE == g_astBoardPinCfg[i].enabled) ||
			(BoardPinModePeripheral != g_astBoardPinCfg[i].run_mode))
		{
			continue;
		}

		reference_count = 0u;
		for (j = 0u; j < BoardAdcIdCount; j++)
		{
			if ((TRUE == g_astBoardAdcCfg[j].enabled) &&
				(i == g_astBoardAdcCfg[j].pin_id))
			{
				reference_count++;
			}
		}
		for (j = 0u; j < BoardIicBusIdCount; j++)
		{
			if (TRUE == g_astBoardIicCfg[j].enabled)
			{
				if (i == g_astBoardIicCfg[j].scl_pin_id)
				{
					reference_count++;
				}
				if (i == g_astBoardIicCfg[j].sda_pin_id)
				{
					reference_count++;
				}
			}
		}
		for (j = 0u; j < BoardCaptureIdCount; j++)
		{
			if ((TRUE == g_astBoardCaptureCfg[j].enabled) &&
				(i == g_astBoardCaptureCfg[j].pin_id))
			{
				reference_count++;
			}
		}
		if (TRUE == g_stcBoardTm3100Cfg.enabled)
		{
			if ((i == g_stcBoardTm3100Cfg.sdi_pin_id) ||
				(i == g_stcBoardTm3100Cfg.clk_pin_id) ||
				(i == g_stcBoardTm3100Cfg.le_pin_id) ||
				(i == g_stcBoardTm3100Cfg.oe_pin_id))
			{
				reference_count++;
			}
		}

		if (1u != reference_count)
		{
			return ErrorInvalidParameter;
		}
	}

	for (i = 0u; i < BoardAdcIdCount; i++)
	{
		if ((TRUE == g_astBoardAdcCfg[i].enabled) &&
			(FALSE == Board_IsEnabledPeripheralPin(g_astBoardAdcCfg[i].pin_id)))
		{
			return ErrorInvalidParameter;
		}
		for (j = (uint8_t)(i + 1u); j < BoardAdcIdCount; j++)
		{
			if ((TRUE == g_astBoardAdcCfg[i].enabled) &&
				(TRUE == g_astBoardAdcCfg[j].enabled) &&
				(g_astBoardAdcCfg[i].channel == g_astBoardAdcCfg[j].channel))
			{
				return ErrorInvalidParameter;
			}
		}
	}

	for (i = 0u; i < BoardIicBusIdCount; i++)
	{
		if ((TRUE == g_astBoardIicCfg[i].enabled) &&
			((FALSE == Board_IsEnabledPeripheralPin(g_astBoardIicCfg[i].scl_pin_id)) ||
			 (FALSE == Board_IsEnabledPeripheralPin(g_astBoardIicCfg[i].sda_pin_id))))
		{
			return ErrorInvalidParameter;
		}
	}

	for (i = 0u; i < BoardCaptureIdCount; i++)
	{
		if ((TRUE == g_astBoardCaptureCfg[i].enabled) &&
			(FALSE == Board_IsEnabledPeripheralPin(g_astBoardCaptureCfg[i].pin_id)))
		{
			return ErrorInvalidParameter;
		}
		for (j = (uint8_t)(i + 1u); j < BoardCaptureIdCount; j++)
		{
			if ((TRUE == g_astBoardCaptureCfg[i].enabled) &&
				(TRUE == g_astBoardCaptureCfg[j].enabled) &&
				(g_astBoardCaptureCfg[i].channel == g_astBoardCaptureCfg[j].channel))
			{
				return ErrorInvalidParameter;
			}
		}
	}

	return Ok;
}
