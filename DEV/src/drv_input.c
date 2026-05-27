#include "drv_input.h"

typedef struct stc_drv_input_cfg
{
	en_bsp_gpio_id_t gpio_id;
	boolean_t active_level;
	uint16_t debounce_ms;
} stc_drv_input_cfg_t;

typedef struct stc_drv_input_state
{
	boolean_t raw_level;
	boolean_t active;
	uint16_t stable_cnt;
} stc_drv_input_state_t;

static const stc_drv_input_cfg_t s_astDrvInputCfg[DrvInputIdCount] = {
	[DrvInputIdIgn] = {.gpio_id = BspGpioIdIgn,
					   .active_level = TRUE,
					   .debounce_ms = 500u},
	[DrvInputIdLeftTurn] = {.gpio_id = BspGpioIdLeftTurn,
							.active_level = TRUE,
							.debounce_ms = 10u},
	[DrvInputIdHighBeam] = {.gpio_id = BspGpioIdHighBeam,
							.active_level = TRUE,
							.debounce_ms = 10u},
	[DrvInputIdRightTurn] = {.gpio_id = BspGpioIdRightTurn,
							 .active_level = TRUE,
							 .debounce_ms = 10u},
	[DrvInputIdPositionLamp] = {.gpio_id = BspGpioIdPositionLamp,
								.active_level = TRUE,
								.debounce_ms = 10u},
};

static stc_drv_input_state_t s_astDrvInputState[DrvInputIdCount];
static boolean_t s_bDrvInputInited = FALSE;

static en_result_t DrvInput_CheckId(en_drv_input_id_t id)
{
	if (id >= DrvInputIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static boolean_t DrvInput_RawToActive(en_drv_input_id_t id, boolean_t raw_level)
{
	return (raw_level == s_astDrvInputCfg[id].active_level) ? TRUE : FALSE;
}

static en_result_t DrvInput_InitImpl(void)
{
	uint8_t i;
	boolean_t raw_level;

	for (i = 0u; i < DrvInputIdCount; i++)
	{
		raw_level = Bsp_Gpio_Read(s_astDrvInputCfg[i].gpio_id);
		s_astDrvInputState[i].raw_level = raw_level;
		s_astDrvInputState[i].active =
			DrvInput_RawToActive((en_drv_input_id_t)i, raw_level);
		s_astDrvInputState[i].stable_cnt = 0u;
	}

	s_bDrvInputInited = TRUE;

	return Ok;
}

static void DrvInput_Task1msImpl(void)
{
	uint8_t i;
	boolean_t raw_level;
	boolean_t active;

	if (FALSE == s_bDrvInputInited)
	{
		return;
	}

	for (i = 0u; i < DrvInputIdCount; i++)
	{
		raw_level = Bsp_Gpio_Read(s_astDrvInputCfg[i].gpio_id);
		active = DrvInput_RawToActive((en_drv_input_id_t)i, raw_level);

		if (active == s_astDrvInputState[i].active)
		{
			s_astDrvInputState[i].stable_cnt = 0u;
			s_astDrvInputState[i].raw_level = raw_level;
			continue;
		}

		if (s_astDrvInputState[i].stable_cnt < s_astDrvInputCfg[i].debounce_ms)
		{
			s_astDrvInputState[i].stable_cnt++;
		}
		else
		{
			s_astDrvInputState[i].active = active;
			s_astDrvInputState[i].raw_level = raw_level;
			s_astDrvInputState[i].stable_cnt = 0u;
		}
	}
}

static boolean_t DrvInput_IsActiveImpl(en_drv_input_id_t id)
{
	if (Ok != DrvInput_CheckId(id))
	{
		return FALSE;
	}

	return s_astDrvInputState[id].active;
}

static boolean_t DrvInput_ReadRawImpl(en_drv_input_id_t id)
{
	if (Ok != DrvInput_CheckId(id))
	{
		return FALSE;
	}

	return Bsp_Gpio_Read(s_astDrvInputCfg[id].gpio_id);
}

static const stc_drv_input_ops_t s_stcDrvInputOps = {
	.init = DrvInput_InitImpl,
	.task_1ms = DrvInput_Task1msImpl,
	.is_active = DrvInput_IsActiveImpl,
	.read_raw = DrvInput_ReadRawImpl,
};

stc_drv_input_t g_stcDrvInput = {
	.ops = &s_stcDrvInputOps,
	.count = DrvInputIdCount,
	.reserved = NULL,
};

en_result_t DRV_Input_Init(void)
{
	return g_stcDrvInput.ops->init();
}

void DRV_Input_Task1ms(void)
{
	g_stcDrvInput.ops->task_1ms();
}

boolean_t DRV_Input_IsActive(en_drv_input_id_t id)
{
	return g_stcDrvInput.ops->is_active(id);
}

boolean_t DRV_Input_ReadRaw(en_drv_input_id_t id)
{
	return g_stcDrvInput.ops->read_raw(id);
}
