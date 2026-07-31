#include "drv_input.h"

typedef struct stc_drv_input_cfg
{
	boolean_t enabled;
	en_bsp_gpio_id_t gpio_id;
	boolean_t active_level;
	boolean_t initialize_from_input;
	boolean_t initial_active;
	uint16_t assert_delay_ms;
	uint16_t release_delay_ms;
} stc_drv_input_cfg_t;

typedef struct stc_drv_input_state
{
	boolean_t raw_level;
	boolean_t active;
	uint16_t stable_cnt;
} stc_drv_input_state_t;

static const stc_drv_input_cfg_t s_astDrvInputCfg[DrvInputIdCount] = {
	[DrvInputIdPositionLamp] = {.enabled = TRUE,
								.gpio_id = BspGpioIdPositionLamp,
								.active_level = TRUE,
								.initialize_from_input = TRUE,
								.initial_active = FALSE,
								.assert_delay_ms = 10u,
								.release_delay_ms = 10u},
	[DrvInputIdGearN] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGearN,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdGear1] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGear1,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdGear2] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGear2,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdGear3] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGear3,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdGear4] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGear4,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdGear5] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGear5,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdGear6] = {.enabled = TRUE,
						 .gpio_id = BspGpioIdGear6,
						 .active_level = FALSE,
						 .initialize_from_input = TRUE,
						 .assert_delay_ms = 10u,
						 .release_delay_ms = 10u},
	[DrvInputIdEnginefault] = {.enabled = TRUE,
							   .gpio_id = BspGpioIdEnginefault,
							   .active_level = FALSE,
							   .initialize_from_input = TRUE,
							   .assert_delay_ms = 10u,
							   .release_delay_ms = 10u},
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
		if (FALSE == s_astDrvInputCfg[i].enabled)
		{
			continue;
		}
		raw_level = Bsp_Gpio_Read(s_astDrvInputCfg[i].gpio_id);
		s_astDrvInputState[i].raw_level = raw_level;
		if (TRUE == s_astDrvInputCfg[i].initialize_from_input)
		{
			s_astDrvInputState[i].active =
				DrvInput_RawToActive((en_drv_input_id_t)i, raw_level);
		}
		else
		{
			s_astDrvInputState[i].active = s_astDrvInputCfg[i].initial_active;
		}
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
	uint16_t delay_ms;

	if (FALSE == s_bDrvInputInited)
	{
		return;
	}

	for (i = 0u; i < DrvInputIdCount; i++)
	{
		if (FALSE == s_astDrvInputCfg[i].enabled)
		{
			continue;
		}
		raw_level = Bsp_Gpio_Read(s_astDrvInputCfg[i].gpio_id);
		active = DrvInput_RawToActive((en_drv_input_id_t)i, raw_level);

		if (active == s_astDrvInputState[i].active)
		{
			s_astDrvInputState[i].stable_cnt = 0u;
			s_astDrvInputState[i].raw_level = raw_level;
			continue;
		}

		delay_ms = active ? s_astDrvInputCfg[i].assert_delay_ms
						  : s_astDrvInputCfg[i].release_delay_ms;
		if ((0u == delay_ms) ||
			((uint16_t)(s_astDrvInputState[i].stable_cnt + 1u) >= delay_ms))
		{
			s_astDrvInputState[i].active = active;
			s_astDrvInputState[i].raw_level = raw_level;
			s_astDrvInputState[i].stable_cnt = 0u;
		}
		else
		{
			s_astDrvInputState[i].stable_cnt++;
		}
	}
}

static boolean_t DrvInput_IsActiveImpl(en_drv_input_id_t id)
{
	if ((Ok != DrvInput_CheckId(id)) || (FALSE == s_astDrvInputCfg[id].enabled))
	{
		return FALSE;
	}

	return s_astDrvInputState[id].active;
}

static boolean_t DrvInput_ReadRawImpl(en_drv_input_id_t id)
{
	if ((Ok != DrvInput_CheckId(id)) || (FALSE == s_astDrvInputCfg[id].enabled))
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

/**
 * @brief 初始化输入驱动模块。
 * @return en_result_t 初始化结果，`Ok` 表示成功。
 */
en_result_t DRV_Input_Init(void)
{
	return g_stcDrvInput.ops->init();
}

/**
 * @brief 执行输入驱动 1ms 周期任务。
 *
 * 负责采样输入状态并处理去抖逻辑。
 */
void DRV_Input_Task1ms(void)
{
	g_stcDrvInput.ops->task_1ms();
}

/**
 * @brief 获取指定输入通道的当前有效状态。
 * @param id 输入通道 ID。
 * @return boolean_t `TRUE` 表示通道处于有效状态，`FALSE` 表示无效。
 */
boolean_t DRV_Input_IsActive(en_drv_input_id_t id)
{
	return g_stcDrvInput.ops->is_active(id);
}

/**
 * @brief 读取指定输入通道的原始电平。
 * @param id 输入通道 ID。
 * @return boolean_t 原始 GPIO 电平，`TRUE` 表示高电平，`FALSE` 表示低电平。
 */
boolean_t DRV_Input_ReadRaw(en_drv_input_id_t id)
{
	return g_stcDrvInput.ops->read_raw(id);
}
