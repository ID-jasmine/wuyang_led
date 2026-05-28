#ifndef __BSP_TIME_CAPTURE_H__
#define __BSP_TIME_CAPTURE_H__

#include "ddl.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		BSP_TIM3_CAP_CH0A = 0,
		BSP_TIM3_CAP_CH0B,
		BSP_TIM3_CAP_CH1A,
		BSP_TIM3_CAP_CH1B,
		BSP_TIM3_CAP_CH2A,
		BSP_TIM3_CAP_CH2B,
		BSP_TIM3_CAP_CH_COUNT
	} bsp_tim3_cap_ch_t;

	typedef void (*bsp_tim3_cap_cb_t)(bsp_tim3_cap_ch_t ch, uint32_t timestamp,
									  void *user_data);

	typedef enum
	{
		BSP_TIM3id_speed = 0,
		BSP_TIM3id_rpm,
		BSP_TIM3id_COUNT,
	} en_bsp_tim3_id_t;

	typedef struct
	{
		bsp_tim3_cap_ch_t ch;
		en_gpio_port_t port;
		en_gpio_pin_t pin;
		en_gpio_af_t af;
	} bsp_tim3_cap_pin_t;

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
