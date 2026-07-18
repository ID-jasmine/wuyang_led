#ifndef __BSP_TIME_CAPTURE_H__
#define __BSP_TIME_CAPTURE_H__

#include "board_config.h"
#include "ddl.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef en_board_capture_channel_t bsp_tim3_cap_ch_t;

#define BSP_TIM3_CAP_CH0A BoardCaptureChannel0A
#define BSP_TIM3_CAP_CH0B BoardCaptureChannel0B
#define BSP_TIM3_CAP_CH1A BoardCaptureChannel1A
#define BSP_TIM3_CAP_CH1B BoardCaptureChannel1B
#define BSP_TIM3_CAP_CH2A BoardCaptureChannel2A
#define BSP_TIM3_CAP_CH2B BoardCaptureChannel2B
#define BSP_TIM3_CAP_CH_COUNT (BoardCaptureChannel2B + 1u)

	typedef void (*bsp_tim3_cap_cb_t)(bsp_tim3_cap_ch_t ch, uint32_t timestamp,
									  void *user_data);

	typedef en_board_capture_id_t en_bsp_tim3_id_t;

#define BSP_TIM3id_speed BoardCaptureIdSpeed
#define BSP_TIM3id_rpm   BoardCaptureIdRpm
#define BSP_TIM3id_COUNT BoardCaptureIdCount

	typedef struct
	{
		uint32_t timer_clk_hz;
		bsp_tim3_cap_cb_t callback;
		void *user_data;
	} bsp_tim3_capture_t;

	en_result_t BSP_TimeCapture_Init(void);
	en_result_t BSP_TimeCapture_RegisterCallback(bsp_tim3_cap_cb_t callback,
												 void *user_data);
	uint32_t BSP_TimeCapture_GetTimerClockHz(void);
	void BSP_TimeCapture_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
