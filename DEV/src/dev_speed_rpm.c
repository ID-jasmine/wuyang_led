#include "dev_speed_rpm.h"
#include "bsp_time_capture.h"

#define DEV_SPEED_RPM_MIN_WINDOW_TICKS	  (150000u)
#define DEV_SPEED_RPM_MIN_PULSE_COUNT	  (24u)
#define DEV_SPEED_RPM_GATE_FAST_MS		  (50u)
#define DEV_SPEED_RPM_GATE_MID_MS		  (50u)
#define DEV_SPEED_RPM_GATE_SLOW_MS		  (1000u)
#define DEV_SPEED_RPM_GATE_FAST_PULSES	  (15u)
#define DEV_SPEED_RPM_GATE_MID_PULSES	  (10u)
#define DEV_SPEED_RPM_GATE_AVG_COUNT	  (3u)
#define DEV_SPEED_RPM_TIMEOUT_MS		  (1000u)
#define DEV_SPEED_RPM_ADAPTIVE_FILTER_SHIFT (1u)
#define DEV_SPEED_RPM_MIN_VALID_DELTA_US  (4000u)
#define DEV_SPEED_RPM_GATE_SPIKE_NUMERATOR	  (11u)
#define DEV_SPEED_RPM_GATE_SPIKE_DENOMINATOR	  (10u)
#define DEV_SPEED_RPM_GATE_SPIKE_CONFIRM_COUNT (2u)
#define DEV_SPEED_RPM_GATE_SPIKE_MATCH_NUMERATOR (11u)
#define DEV_SPEED_RPM_GATE_SPIKE_MATCH_DENOMINATOR (10u)

typedef struct
{
	volatile uint32_t first_tick;
	volatile uint32_t last_tick;
	volatile uint16_t pulse_count;
	volatile uint16_t gate_pulse_count;
	volatile uint16_t gate_elapsed_ms;
	volatile uint32_t total_pulse_count;
	volatile uint16_t timeout_count;
	volatile boolean_t started;
	volatile uint32_t last_valid_timestamp;
	volatile boolean_t has_valid_timestamp;
	uint32_t gate_freq_samples[DEV_SPEED_RPM_GATE_AVG_COUNT];
	uint32_t gate_freq_sum;
	uint8_t gate_freq_index;
	uint8_t gate_freq_count;
	uint32_t gate_spike_candidate_freq;
	uint8_t gate_spike_candidate_count;
	uint32_t freq_mhz[DevSpeedRpmMeasureCount];
	boolean_t valid[DevSpeedRpmMeasureCount];
} stc_dev_speed_rpm_state_t;

typedef struct
{
	uint32_t first_tick;
	uint32_t last_tick;
	uint16_t pulse_count;
	uint16_t gate_pulse_count;
	uint16_t elapsed_ms;
	boolean_t need_calc[DevSpeedRpmMeasureCount];
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

static en_result_t DevSpeedRpm_CheckMeasure(en_dev_speed_rpm_measure_t measure)
{
	if (measure >= DevSpeedRpmMeasureCount)
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

static void DevSpeedRpm_ResetAdaptiveWindow(stc_dev_speed_rpm_state_t *state,
											uint32_t timestamp)
{
	state->first_tick = timestamp;
	state->last_tick = timestamp;
	state->pulse_count = 0u;
}

static void DevSpeedRpm_ResetGateWindow(stc_dev_speed_rpm_state_t *state)
{
	state->gate_pulse_count = 0u;
	state->gate_elapsed_ms = 0u;
}

static void DevSpeedRpm_ResetGateAverage(stc_dev_speed_rpm_state_t *state)
{
	uint8_t i;

	for (i = 0u; i < DEV_SPEED_RPM_GATE_AVG_COUNT; i++)
	{
		state->gate_freq_samples[i] = 0u;
	}
	state->gate_freq_sum = 0u;
	state->gate_freq_index = 0u;
	state->gate_freq_count = 0u;
	state->gate_spike_candidate_freq = 0u;
	state->gate_spike_candidate_count = 0u;
}

static void DevSpeedRpm_SetInvalid(en_dev_speed_rpm_id_t id)
{
	uint8_t i;

	for (i = 0u; i < DevSpeedRpmMeasureCount; i++)
	{
		s_astDevSpeedRpmState[id].freq_mhz[i] = 0u;
		s_astDevSpeedRpmState[id].valid[i] = FALSE;
	}
	DevSpeedRpm_ResetGateAverage(&s_astDevSpeedRpmState[id]);
}

static boolean_t
DevSpeedRpm_IsCaptureDeltaTooShort(const stc_dev_speed_rpm_state_t *state,
								   uint32_t timestamp)
{
	uint32_t delta_ticks;
	uint32_t short_delta_ticks;

	if ((FALSE == state->has_valid_timestamp) || (0u == s_u32TimerClockHz))
	{
		return FALSE;
	}

	delta_ticks = timestamp - state->last_valid_timestamp;
	short_delta_ticks =
		(s_u32TimerClockHz * DEV_SPEED_RPM_MIN_VALID_DELTA_US) / 1000000u;
	if ((delta_ticks > 0u) && (delta_ticks < short_delta_ticks))
	{
		return TRUE;
	}

	return FALSE;
}

static void DevSpeedRpm_UpdateValidCaptureState(stc_dev_speed_rpm_state_t *state,
												uint32_t timestamp)
{
	state->last_valid_timestamp = timestamp;
	state->has_valid_timestamp = TRUE;
}

static boolean_t DevSpeedRpm_ShouldIgnoreGateSpike(stc_dev_speed_rpm_state_t *state,
												   uint32_t new_freq_mhz)
{
	uint32_t current_freq_mhz;
	uint32_t spike_threshold_mhz;
	uint32_t candidate_low_mhz;
	uint32_t candidate_high_mhz;

	if ((FALSE == state->valid[DevSpeedRpmMeasureGate]) ||
		(0u == state->freq_mhz[DevSpeedRpmMeasureGate]))
	{
		state->gate_spike_candidate_freq = 0u;
		state->gate_spike_candidate_count = 0u;
		return FALSE;
	}

	current_freq_mhz = state->freq_mhz[DevSpeedRpmMeasureGate];
	spike_threshold_mhz =
		(uint32_t)(((uint64_t)current_freq_mhz *
					DEV_SPEED_RPM_GATE_SPIKE_NUMERATOR) /
				   DEV_SPEED_RPM_GATE_SPIKE_DENOMINATOR);
	if (new_freq_mhz <= spike_threshold_mhz)
	{
		state->gate_spike_candidate_freq = 0u;
		state->gate_spike_candidate_count = 0u;
		return FALSE;
	}

	if (0u != state->gate_spike_candidate_freq)
	{
		candidate_low_mhz =
			(uint32_t)(((uint64_t)state->gate_spike_candidate_freq *
						DEV_SPEED_RPM_GATE_SPIKE_DENOMINATOR) /
					   DEV_SPEED_RPM_GATE_SPIKE_MATCH_NUMERATOR);
		candidate_high_mhz =
			(uint32_t)(((uint64_t)state->gate_spike_candidate_freq *
						DEV_SPEED_RPM_GATE_SPIKE_MATCH_NUMERATOR) /
					   DEV_SPEED_RPM_GATE_SPIKE_MATCH_DENOMINATOR);
		if ((new_freq_mhz < candidate_low_mhz) ||
			(new_freq_mhz > candidate_high_mhz))
		{
			state->gate_spike_candidate_freq = new_freq_mhz;
			state->gate_spike_candidate_count = 0u;
		}
	}
	else
	{
		state->gate_spike_candidate_freq = new_freq_mhz;
	}

	if (state->gate_spike_candidate_count < DEV_SPEED_RPM_GATE_SPIKE_CONFIRM_COUNT)
	{
		state->gate_spike_candidate_count++;
	}

	if (state->gate_spike_candidate_count < DEV_SPEED_RPM_GATE_SPIKE_CONFIRM_COUNT)
	{
		return TRUE;
	}

	DevSpeedRpm_ResetGateAverage(state);
	return FALSE;
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
	if (TRUE == DevSpeedRpm_IsCaptureDeltaTooShort(state, timestamp))
	{
		return;
	}
	DevSpeedRpm_UpdateValidCaptureState(state, timestamp);

	state->total_pulse_count++;
	if (FALSE == state->started)
	{
		state->started = TRUE;
		DevSpeedRpm_ResetAdaptiveWindow(state, timestamp);
		DevSpeedRpm_ResetGateWindow(state);
	}
	else
	{
		if (state->gate_pulse_count < 0xFFFFu)
		{
			state->gate_pulse_count++;
		}
		state->last_tick = timestamp;
		if (state->pulse_count < 0xFFFFu)
		{
			state->pulse_count++;
		}
	}

	state->timeout_count = 0u;
}

static void DevSpeedRpm_UpdateFreq(en_dev_speed_rpm_id_t id,
								   en_dev_speed_rpm_measure_t measure,
								   uint32_t new_freq_mhz)
{
	if (DevSpeedRpmMeasureGate == measure)
	{
		stc_dev_speed_rpm_state_t *state = &s_astDevSpeedRpmState[id];

		if (TRUE == DevSpeedRpm_ShouldIgnoreGateSpike(state, new_freq_mhz))
		{
			return;
		}

		if (state->gate_freq_count >= DEV_SPEED_RPM_GATE_AVG_COUNT)
		{
			state->gate_freq_sum -= state->gate_freq_samples[state->gate_freq_index];
		}
		else
		{
			state->gate_freq_count++;
		}

		state->gate_freq_samples[state->gate_freq_index] = new_freq_mhz;
		state->gate_freq_sum += new_freq_mhz;
		state->gate_freq_index++;
		if (state->gate_freq_index >= DEV_SPEED_RPM_GATE_AVG_COUNT)
		{
			state->gate_freq_index = 0u;
		}

		s_astDevSpeedRpmState[id].freq_mhz[measure] =
			(state->gate_freq_sum + (state->gate_freq_count / 2u)) /
			state->gate_freq_count;
		s_astDevSpeedRpmState[id].valid[measure] = TRUE;
		return;
	}

	if ((DevSpeedRpmIdSpeed == id) && (TRUE == s_astDevSpeedRpmState[id].valid[measure]))
	{
		s_astDevSpeedRpmState[id].freq_mhz[measure] =
			(uint32_t)((((uint64_t)s_astDevSpeedRpmState[id].freq_mhz[measure] *
						 ((1u << DEV_SPEED_RPM_ADAPTIVE_FILTER_SHIFT) - 1u)) +
						new_freq_mhz +
						(1u << (DEV_SPEED_RPM_ADAPTIVE_FILTER_SHIFT - 1u))) >>
					   DEV_SPEED_RPM_ADAPTIVE_FILTER_SHIFT);
	}
	else
	{
		s_astDevSpeedRpmState[id].freq_mhz[measure] = new_freq_mhz;
	}

	s_astDevSpeedRpmState[id].valid[measure] = TRUE;
}

static boolean_t DevSpeedRpm_IsGateReady(const stc_dev_speed_rpm_state_t *state)
{
	if (state->gate_elapsed_ms >= DEV_SPEED_RPM_GATE_SLOW_MS)
	{
		return TRUE;
	}

	if ((state->gate_elapsed_ms >= DEV_SPEED_RPM_GATE_MID_MS) &&
		(state->gate_pulse_count >= DEV_SPEED_RPM_GATE_MID_PULSES))
	{
		return TRUE;
	}

	if ((state->gate_elapsed_ms >= DEV_SPEED_RPM_GATE_FAST_MS) &&
		(state->gate_pulse_count >= DEV_SPEED_RPM_GATE_FAST_PULSES))
	{
		return TRUE;
	}

	return FALSE;
}

static void DevSpeedRpm_CalcGateFreq(en_dev_speed_rpm_id_t id,
									 const stc_dev_speed_rpm_snapshot_t *snapshot)
{
	uint32_t freq_mhz;

	if (0u == snapshot->elapsed_ms)
	{
		return;
	}

	if (0u == snapshot->gate_pulse_count)
	{
		s_astDevSpeedRpmState[id].freq_mhz[DevSpeedRpmMeasureGate] = 0u;
		s_astDevSpeedRpmState[id].valid[DevSpeedRpmMeasureGate] = FALSE;
		return;
	}

	freq_mhz = (uint32_t)(((uint64_t)snapshot->gate_pulse_count * 1000000u) /
						  snapshot->elapsed_ms);
	DevSpeedRpm_UpdateFreq(id, DevSpeedRpmMeasureGate, freq_mhz);
}

static void DevSpeedRpm_CalcAdaptiveFreq(en_dev_speed_rpm_id_t id,
										 const stc_dev_speed_rpm_snapshot_t *snapshot)
{
	uint32_t elapsed_ticks;
	uint32_t freq_mhz;

	elapsed_ticks = snapshot->last_tick - snapshot->first_tick;
	if ((0u == elapsed_ticks) || (0u == snapshot->pulse_count))
	{
		return;
	}

	freq_mhz = (uint32_t)(((uint64_t)snapshot->pulse_count * s_u32TimerClockHz * 1000u) /
						  elapsed_ticks);
	DevSpeedRpm_UpdateFreq(id, DevSpeedRpmMeasureAdaptive, freq_mhz);
}

static void DevSpeedRpm_CalcFreq(en_dev_speed_rpm_id_t id,
								 const stc_dev_speed_rpm_snapshot_t *snapshot)
{
	if (TRUE == snapshot->timeout)
	{
		DevSpeedRpm_SetInvalid(id);
		return;
	}

	if (TRUE == snapshot->need_calc[DevSpeedRpmMeasureGate])
	{
		DevSpeedRpm_CalcGateFreq(id, snapshot);
	}

	if (TRUE == snapshot->need_calc[DevSpeedRpmMeasureAdaptive])
	{
		DevSpeedRpm_CalcAdaptiveFreq(id, snapshot);
	}
}

static void DevSpeedRpm_TakeSnapshot(en_dev_speed_rpm_id_t id,
									 stc_dev_speed_rpm_snapshot_t *snapshot)
{
	uint32_t primask;
	uint32_t elapsed_ticks;
	stc_dev_speed_rpm_state_t *state;

	state = &s_astDevSpeedRpmState[id];
	snapshot->need_calc[DevSpeedRpmMeasureGate] = FALSE;
	snapshot->need_calc[DevSpeedRpmMeasureAdaptive] = FALSE;
	snapshot->timeout = FALSE;
	snapshot->gate_pulse_count = 0u;
	snapshot->elapsed_ms = 0u;

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
			DevSpeedRpm_ResetAdaptiveWindow(state, state->last_tick);
			DevSpeedRpm_ResetGateWindow(state);
			snapshot->timeout = TRUE;
			__set_PRIMASK(primask);
			return;
		}

		if (state->gate_elapsed_ms < DEV_SPEED_RPM_GATE_SLOW_MS)
		{
			state->gate_elapsed_ms++;
		}
		if (TRUE == DevSpeedRpm_IsGateReady(state))
		{
			snapshot->gate_pulse_count = state->gate_pulse_count;
			snapshot->elapsed_ms = state->gate_elapsed_ms;
			snapshot->need_calc[DevSpeedRpmMeasureGate] = TRUE;
			DevSpeedRpm_ResetGateWindow(state);
		}

		elapsed_ticks = state->last_tick - state->first_tick;
		if (((elapsed_ticks >= DEV_SPEED_RPM_MIN_WINDOW_TICKS) ||
			 (state->pulse_count >= DEV_SPEED_RPM_MIN_PULSE_COUNT)) &&
			(state->pulse_count > 0u))
		{
			snapshot->first_tick = state->first_tick;
			snapshot->last_tick = state->last_tick;
			snapshot->pulse_count = state->pulse_count;
			snapshot->need_calc[DevSpeedRpmMeasureAdaptive] = TRUE;
			DevSpeedRpm_ResetAdaptiveWindow(state, state->last_tick);
		}
	}
	else
	{
		state->started = FALSE;
		state->timeout_count = DEV_SPEED_RPM_TIMEOUT_MS;
		DevSpeedRpm_ResetAdaptiveWindow(state, state->last_tick);
		DevSpeedRpm_ResetGateWindow(state);
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
		s_astDevSpeedRpmState[i].gate_pulse_count = 0u;
		s_astDevSpeedRpmState[i].gate_elapsed_ms = 0u;
		s_astDevSpeedRpmState[i].total_pulse_count = 0u;
		s_astDevSpeedRpmState[i].timeout_count = 0u;
		s_astDevSpeedRpmState[i].started = FALSE;
		s_astDevSpeedRpmState[i].last_valid_timestamp = 0u;
		s_astDevSpeedRpmState[i].has_valid_timestamp = FALSE;
		DevSpeedRpm_ResetGateAverage(&s_astDevSpeedRpmState[i]);
		DevSpeedRpm_SetInvalid((en_dev_speed_rpm_id_t)i);
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

static uint32_t
DevSpeedRpm_GetFreqMilliHzByMeasureImpl(en_dev_speed_rpm_id_t id,
										en_dev_speed_rpm_measure_t measure)
{
	uint32_t primask;
	uint32_t freq_mhz;

	if ((Ok != DevSpeedRpm_CheckId(id)) || (Ok != DevSpeedRpm_CheckMeasure(measure)))
	{
		return 0u;
	}

	primask = __get_PRIMASK();
	__disable_irq();
	freq_mhz = (TRUE == s_astDevSpeedRpmState[id].valid[measure])
				   ? s_astDevSpeedRpmState[id].freq_mhz[measure]
				   : 0u;
	__set_PRIMASK(primask);

	return freq_mhz;
}

static uint32_t DevSpeedRpm_GetFreqMilliHzImpl(en_dev_speed_rpm_id_t id)
{
	return DevSpeedRpm_GetFreqMilliHzByMeasureImpl(id, DEV_SPEED_RPM_DEFAULT_MEASURE);
}

static uint32_t DevSpeedRpm_GetPulseCountImpl(en_dev_speed_rpm_id_t id)
{
	uint32_t primask;
	uint32_t pulse_count;

	if (Ok != DevSpeedRpm_CheckId(id))
	{
		return 0u;
	}

	primask = __get_PRIMASK();
	__disable_irq();
	pulse_count = s_astDevSpeedRpmState[id].total_pulse_count;
	__set_PRIMASK(primask);

	return pulse_count;
}

static boolean_t DevSpeedRpm_IsValidByMeasureImpl(en_dev_speed_rpm_id_t id,
												  en_dev_speed_rpm_measure_t measure)
{
	uint32_t primask;
	boolean_t valid;

	if ((Ok != DevSpeedRpm_CheckId(id)) || (Ok != DevSpeedRpm_CheckMeasure(measure)))
	{
		return FALSE;
	}

	primask = __get_PRIMASK();
	__disable_irq();
	valid = s_astDevSpeedRpmState[id].valid[measure];
	__set_PRIMASK(primask);

	return valid;
}

static boolean_t DevSpeedRpm_IsValidImpl(en_dev_speed_rpm_id_t id)
{
	return DevSpeedRpm_IsValidByMeasureImpl(id, DEV_SPEED_RPM_DEFAULT_MEASURE);
}

static const stc_dev_speed_rpm_ops_t s_stcDevSpeedRpmOps = {
	.init = DevSpeedRpm_InitImpl,
	.task_1ms = DevSpeedRpm_Task1msImpl,
	.get_freq_mhz = DevSpeedRpm_GetFreqMilliHzImpl,
	.get_freq_mhz_by_measure = DevSpeedRpm_GetFreqMilliHzByMeasureImpl,
	.get_pulse_count = DevSpeedRpm_GetPulseCountImpl,
	.is_valid = DevSpeedRpm_IsValidImpl,
	.is_valid_by_measure = DevSpeedRpm_IsValidByMeasureImpl,
};

stc_dev_speed_rpm_t g_stcDevSpeedRpm = {
	.ops = &s_stcDevSpeedRpmOps,
	.count = DevSpeedRpmIdCount,
	.reserved = NULL,
};

/**
 * @brief 初始化车速和转速频率检测模块。
 * @return en_result_t 初始化结果。
 */
en_result_t DEV_SpeedRpm_Init(void)
{
	return g_stcDevSpeedRpm.ops->init();
}

/**
 * @brief 执行车速和转速检测 1ms 周期任务。
 *
 * 负责处理快照、超时判断以及频率更新。
 */
void DEV_SpeedRpm_Task1ms(void)
{
	g_stcDevSpeedRpm.ops->task_1ms();
}

/**
 * @brief 获取指定通道的默认频率值。
 * @param id 通道 ID。
 * @return uint32_t 频率值，单位为 mHz；无效通道时返回 `0`。
 */
uint32_t DEV_SpeedRpm_GetFreqMilliHz(en_dev_speed_rpm_id_t id)
{
	return g_stcDevSpeedRpm.ops->get_freq_mhz(id);
}

uint32_t DEV_SpeedRpm_GetFreqMilliHzByMeasure(en_dev_speed_rpm_id_t id,
											  en_dev_speed_rpm_measure_t measure)
{
	return g_stcDevSpeedRpm.ops->get_freq_mhz_by_measure(id, measure);
}

uint32_t DEV_SpeedRpm_GetPulseCount(en_dev_speed_rpm_id_t id)
{
	return g_stcDevSpeedRpm.ops->get_pulse_count(id);
}

/**
 * @brief 查询指定通道的默认频率数据是否有效。
 * @param id 通道 ID。
 * @return boolean_t `TRUE` 表示当前频率有效，`FALSE` 表示无效或已超时。
 */
boolean_t DEV_SpeedRpm_IsValid(en_dev_speed_rpm_id_t id)
{
	return g_stcDevSpeedRpm.ops->is_valid(id);
}

boolean_t DEV_SpeedRpm_IsValidByMeasure(en_dev_speed_rpm_id_t id,
										en_dev_speed_rpm_measure_t measure)
{
	return g_stcDevSpeedRpm.ops->is_valid_by_measure(id, measure);
}
