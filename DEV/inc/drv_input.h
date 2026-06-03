#ifndef __DRV_INPUT_H__
#define __DRV_INPUT_H__

#include "bsp_gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum en_drv_input_id
	{
		DrvInputIdPositionLamp = 0u,
		DrvInputIdGearN,
		DrvInputIdGear1,
		DrvInputIdGear2,
		DrvInputIdGear3,
		DrvInputIdGear4,
		DrvInputIdGear5,
		DrvInputIdGear6,
		DrvInputIdEnginefault,
		DrvInputIdCount,
	} en_drv_input_id_t;

	typedef struct stc_drv_input_ops
	{
		en_result_t (*init)(void);
		void (*task_1ms)(void);
		boolean_t (*is_active)(en_drv_input_id_t id);
		boolean_t (*read_raw)(en_drv_input_id_t id);
	} stc_drv_input_ops_t;

	typedef struct stc_drv_input
	{
		const stc_drv_input_ops_t *ops;
		uint8_t count;
		void *reserved;
	} stc_drv_input_t;

	extern stc_drv_input_t g_stcDrvInput;

	en_result_t DRV_Input_Init(void);
	void DRV_Input_Task1ms(void);
	boolean_t DRV_Input_IsActive(en_drv_input_id_t id);
	boolean_t DRV_Input_ReadRaw(en_drv_input_id_t id);

#ifdef __cplusplus
}
#endif

#endif
