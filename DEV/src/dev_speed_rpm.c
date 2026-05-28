#include "dev_speed_rpm.h"
#include "bsp_time_capture.h"

#define DEV_SPEED_RPM_MIN_WINDOW_TICKS (100000u)
#define DEV_SPEED_RPM_MIN_PULSE_COUNT  (32u)
#define DEV_SPEED_RPM_TIMEOUT_MS	   (1000u)

typedef struct
{
	volatile uint32_t first_tick;
	volatile uint32_t last_tick;
	volatile uint16_t pulse_count;
	volatile uint16_t timeout_count;
	volatile boolean_t started;
	uint32_t freq_mhz;
	boolean_t valid;
} stc_dev_speed_rpm_state_t;

typedef struct
{
	uint32_t first_tick;
	uint32_t last_tick;
	uint16_t pulse_count;
	boolean_t need_calc;
	boolean_t timeout;
} stc_dev_speed_rpm_snapshot_t;

static stc_dev_speed_rpm_state_t s_astDevSpeedRpmState[DevSpeedRpmIdCount];
static uint32_t s_u32TimerClockHz = 0u;

static en_result_t DevSpeedRpm_CheckId(en_dev_speed_rpm_id_t id)
{
	if (id >= DevSpeedRpmIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static en_dev_speed_rpm_id_t DevSpeedRpm_CapChToId(bsp_tim3_cap_ch_t ch)
{
	switch (ch)
	{
	case BSP_TIM3_CAP_CH2A:
		return DevSpeedRpmIdSpeed;

	case BSP_TIM3_CAP_CH1A:
		return DevSpeedRpmIdRpm;

	default:
		return DevSpeedRpmIdCount;
	}
}

static void DevSpeedRpm_ResetWindow(stc_dev_speed_rpm_state_t *state, uint32_t timestamp)
{
	state->first_tick = timestamp;
	state->last_tick = timestamp;
	state->pulse_count = 0u;
}

static void DevSpeedRpm_CaptureCallback(bsp_tim3_cap_ch_t ch, uint32_t timestamp,
										void *user_data)
{
	en_dev_speed_rpm_id_t id;
	stc_dev_speed_rpm_state_t *state;

	(void)user_data;

	id = DevSpeedRpm_CapChToId(ch);
	if (DevSpeedRpmIdCount == id)
	{
		return;
	}

	state = &s_astDevSpeedRpmState[id];
	if (FALSE == state->started)
	{
		state->started = TRUE;
		DevSpeedRpm_ResetWindow(state, timestamp);
	}
	else
	{
		state->last_tick = timestamp;
		if (state->pulse_count < 0xFFFFu)
		{
			state->pulse_count++;
		}
	}

	state->timeout_count = 0u;
}

static void DevSpeedRpm_CalcFreq(en_dev_speed_rpm_id_t id,
								 const stc_dev_speed_rpm_snapshot_t *snapshot)
{
	uint32_t elapsed_ticks;
	uint32_t pulse_count;
	uint64_t freq_mhz;

	if (TRUE == snapshot->timeout)
	{
		s_astDevSpeedRpmState[id].freq_mhz = 0u;
		s_astDevSpeedRpmState[id].valid = FALSE;
		return;
	}

	if (FALSE == snapshot->need_calc)
	{
		return;
	}

	elapsed_ticks = snapshot->last_tick - snapshot->first_tick;
	pulse_count = snapshot->pulse_count;

	if ((0u == elapsed_ticks) || (0u == pulse_count))
	{
		return;
	}

	if ((elapsed_ticks < DEV_SPEED_RPM_MIN_WINDOW_TICKS) &&
		(pulse_count < DEV_SPEED_RPM_MIN_PULSE_COUNT))
	{
		return;
	}

	freq_mhz = (uint64_t)pulse_count * s_u32TimerClockHz * 1000u;
	freq_mhz /= elapsed_ticks;

	s_astDevSpeedRpmState[id].freq_mhz = (uint32_t)freq_mhz;
	s_astDevSpeedRpmState[id].valid = TRUE;
}

static void DevSpeedRpm_TakeSnapshot(en_dev_speed_rpm_id_t id,
									 stc_dev_speed_rpm_snapshot_t *snapshot)
{
	uint32_t primask;
	uint32_t elapsed_ticks;
	stc_dev_speed_rpm_state_t *state;

	state = &s_astDevSpeedRpmState[id];
	snapshot->need_calc = FALSE;
	snapshot->timeout = FALSE;

	primask = __get_PRIMASK();
	__disable_irq();

	if (FALSE == state->started)
	{
		if (state->timeout_count < DEV_SPEED_RPM_TIMEOUT_MS)
		{
			state->timeout_count++;
		}
		if (state->timeout_count >= DEV_SPEED_RPM_TIMEOUT_MS)
		{
			snapshot->timeout = TRUE;
		}
	}
	else if (state->timeout_count < DEV_SPEED_RPM_TIMEOUT_MS)
	{
		state->timeout_count++;
		if (state->timeout_count >= DEV_SPEED_RPM_TIMEOUT_MS)
		{
			state->started = FALSE;
			DevSpeedRpm_ResetWindow(state, state->last_tick);
			snapshot->timeout = TRUE;
			__set_PRIMASK(primask);
			return;
		}

		snapshot->first_tick = state->first_tick;
		snapshot->last_tick = state->last_tick;
		snapshot->pulse_count = state->pulse_count;
		elapsed_ticks = snapshot->last_tick - snapshot->first_tick;

		if (((elapsed_ticks >= DEV_SPEED_RPM_MIN_WINDOW_TICKS) ||
			 (snapshot->pulse_count >= DEV_SPEED_RPM_MIN_PULSE_COUNT)) &&
			(snapshot->pulse_count > 0u))
		{
			snapshot->need_calc = TRUE;
			DevSpeedRpm_ResetWindow(state, state->last_tick);
		}
	}
	else
	{
		state->started = FALSE;
		state->timeout_count = DEV_SPEED_RPM_TIMEOUT_MS;
		DevSpeedRpm_ResetWindow(state, state->last_tick);
		snapshot->timeout = TRUE;
	}

	__set_PRIMASK(primask);
}

static en_result_t DevSpeedRpm_InitImpl(void)
{
	uint8_t i;

	for (i = 0u; i < DevSpeedRpmIdCount; i++)
	{
		s_astDevSpeedRpmState[i].first_tick = 0u;
		s_astDevSpeedRpmState[i].last_tick = 0u;
		s_astDevSpeedRpmState[i].pulse_count = 0u;
		s_astDevSpeedRpmState[i].timeout_count = 0u;
		s_astDevSpeedRpmState[i].freq_mhz = 0u;
		s_astDevSpeedRpmState[i].valid = FALSE;
		s_astDevSpeedRpmState[i].started = FALSE;
	}

	(void)BSP_TimeCapture_RegisterCallback(DevSpeedRpm_CaptureCallback, NULL);
	(void)BSP_TimeCapture_Init();
	s_u32TimerClockHz = BSP_TimeCapture_GetTimerClockHz();

	return Ok;
}

static void DevSpeedRpm_Task1msImpl(void)
{
	uint8_t i;
	stc_dev_speed_rpm_snapshot_t snapshot;

	for (i = 0u; i < DevSpeedRpmIdCount; i++)
	{
		DevSpeedRpm_TakeSnapshot((en_dev_speed_rpm_id_t)i, &snapshot);
		DevSpeedRpm_CalcFreq((en_dev_speed_rpm_id_t)i, &snapshot);
	}
}

static uint32_t DevSpeedRpm_GetFreqMilliHzImpl(en_dev_speed_rpm_id_t id)
{
	if (Ok != DevSpeedRpm_CheckId(id))
	{
		return 0u;
	}

	return s_astDevSpeedRpmState[id].freq_mhz;
}

static boolean_t DevSpeedRpm_IsValidImpl(en_dev_speed_rpm_id_t id)
{
	if (Ok != DevSpeedRpm_CheckId(id))
	{
		return FALSE;
	}

	return s_astDevSpeedRpmState[id].valid;
}

static const stc_dev_speed_rpm_ops_t s_stcDevSpeedRpmOps = {
	.init = DevSpeedRpm_InitImpl,
	.task_1ms = DevSpeedRpm_Task1msImpl,
	.get_freq_mhz = DevSpeedRpm_GetFreqMilliHzImpl,
	.is_valid = DevSpeedRpm_IsValidImpl,
};

stc_dev_speed_rpm_t g_stcDevSpeedRpm = {
	.ops = &s_stcDevSpeedRpmOps,
	.count = DevSpeedRpmIdCount,
	.reserved = NULL,
};

/**
 * @brief  车速/转速检测模块初始化（清空状态，注册捕获回调机制）
 * @return en_result_t Ok: 初始化成功
 */
en_result_t DEV_SpeedRpm_Init(void)
{
	return g_stcDevSpeedRpm.ops->init();
}

/**
 * @brief  1ms周期任务：获取最新快照、处理超时判定并计算最新频率（转速）
 */
void DEV_SpeedRpm_Task1ms(void)
{
	g_stcDevSpeedRpm.ops->task_1ms();
}

/**
 * @brief  获取指定通道的频率测试结果（单位：mHz 毫赫兹，即真实的 Hz * 1000）
 * @param  id 通道ID (例如 车速、发动机转速)
 * @return uint32_t 频率值，如果超时或未启动则返回0
 */
uint32_t DEV_SpeedRpm_GetFreqMilliHz(en_dev_speed_rpm_id_t id)
{
	return g_stcDevSpeedRpm.ops->get_freq_mhz(id);
}

/**
 * @brief  获取当前测量的频率数据是否有效
 * @param  id 通道ID (例如 车速、发动机转速)
 * @return boolean_t TRUE: 数据有效，正在正常检测中; FALSE: 数据无效（已超时停止或未就绪）
 */
boolean_t DEV_SpeedRpm_IsValid(en_dev_speed_rpm_id_t id)
{
	return g_stcDevSpeedRpm.ops->is_valid(id);
}
