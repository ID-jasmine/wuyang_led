#ifndef __DEV_SPEED_RPM_H__
#define __DEV_SPEED_RPM_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		DevSpeedRpmIdSpeed = 0u,
		DevSpeedRpmIdRpm,
		DevSpeedRpmIdCount,
	} en_dev_speed_rpm_id_t;

	typedef enum
	{
		DevSpeedRpmMeasureGate = 0u,
		DevSpeedRpmMeasureAdaptive,
		DevSpeedRpmMeasureCount,
	} en_dev_speed_rpm_measure_t;

	typedef struct
	{
		en_result_t (*init)(void);
		void (*task_1ms)(void);
		uint32_t (*get_freq_mhz)(en_dev_speed_rpm_id_t id);
		uint32_t (*get_freq_mhz_by_measure)(en_dev_speed_rpm_id_t id,
											 en_dev_speed_rpm_measure_t measure);
		uint32_t (*get_pulse_count)(en_dev_speed_rpm_id_t id);
		boolean_t (*is_valid)(en_dev_speed_rpm_id_t id);
		boolean_t (*is_valid_by_measure)(en_dev_speed_rpm_id_t id,
										  en_dev_speed_rpm_measure_t measure);
	} stc_dev_speed_rpm_ops_t;

	typedef struct
	{
		const stc_dev_speed_rpm_ops_t *ops;
		uint8_t count;
		void *reserved;
	} stc_dev_speed_rpm_t;

	typedef struct
	{
		uint32_t total_pulse_count;
		uint32_t last_timestamp;
		uint32_t last_delta_ticks;
		uint32_t valid_delta_ticks;
		uint32_t min_delta_ticks;
		uint32_t max_delta_ticks;
		uint16_t short_delta_count;
		uint16_t pulse_count_per_sec;
		boolean_t has_delta;
		boolean_t has_valid_delta;
	} stc_dev_speed_rpm_capture_diag_t;

	extern stc_dev_speed_rpm_t g_stcDevSpeedRpm;

	en_result_t DEV_SpeedRpm_Init(void);
	void DEV_SpeedRpm_Task1ms(void);
	uint32_t DEV_SpeedRpm_GetFreqMilliHz(en_dev_speed_rpm_id_t id);
	uint32_t DEV_SpeedRpm_GetFreqMilliHzByMeasure(en_dev_speed_rpm_id_t id,
												  en_dev_speed_rpm_measure_t measure);
	uint32_t DEV_SpeedRpm_GetPulseCount(en_dev_speed_rpm_id_t id);
	en_result_t DEV_SpeedRpm_GetCaptureDiag(en_dev_speed_rpm_id_t id,
											stc_dev_speed_rpm_capture_diag_t *diag);
	en_result_t DEV_SpeedRpm_ResetCaptureDiag(en_dev_speed_rpm_id_t id);
	boolean_t DEV_SpeedRpm_IsValid(en_dev_speed_rpm_id_t id);
	boolean_t DEV_SpeedRpm_IsValidByMeasure(en_dev_speed_rpm_id_t id,
											en_dev_speed_rpm_measure_t measure);

#ifdef __cplusplus
}
#endif

#endif
