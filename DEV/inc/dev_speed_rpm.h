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

	typedef struct
	{
		en_result_t (*init)(void);
		void (*task_1ms)(void);
		uint32_t (*get_freq_mhz)(en_dev_speed_rpm_id_t id);
		boolean_t (*is_valid)(en_dev_speed_rpm_id_t id);
	} stc_dev_speed_rpm_ops_t;

	typedef struct
	{
		const stc_dev_speed_rpm_ops_t *ops;
		uint8_t count;
		void *reserved;
	} stc_dev_speed_rpm_t;

	extern stc_dev_speed_rpm_t g_stcDevSpeedRpm;

	en_result_t DEV_SpeedRpm_Init(void);
	void DEV_SpeedRpm_Task1ms(void);
	uint32_t DEV_SpeedRpm_GetFreqMilliHz(en_dev_speed_rpm_id_t id);
	boolean_t DEV_SpeedRpm_IsValid(en_dev_speed_rpm_id_t id);

#ifdef __cplusplus
}
#endif

#endif
