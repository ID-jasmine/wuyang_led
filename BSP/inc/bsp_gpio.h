#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum en_bsp_gpio_id
	{
		BspGpioIdIgn = 0u,
		BspGpioIdPower,
		BspGpioIdLeftTurn,
		BspGpioIdHighBeam,
		BspGpioIdRightTurn,
		BspGpioIdPositionLamp,
		BspGpioIdPhotoDetec,
		BspGpioIdSwK1,
		BspGpioIdSwK2,
		BspGpioIdGearN,
		BspGpioIdGear1,
		BspGpioIdGear2,
		BspGpioIdGear3,
		BspGpioIdGear4,
		BspGpioIdGear5,
		BspGpioIdGear6,
		BspGpioIdEnginefault,
		BspGpioIdLEDPower,
		BspGpioIdCount,
	} en_bsp_gpio_id_t;

	typedef enum en_bsp_gpio_dir
	{
		BspGpioDirOut = 0u,
		BspGpioDirIn = 1u,
	} en_bsp_gpio_dir_t;

	typedef struct stc_bsp_gpio_ops
	{
		en_result_t (*init)(en_bsp_gpio_id_t id);
		boolean_t (*read)(en_bsp_gpio_id_t id);
		en_result_t (*write)(en_bsp_gpio_id_t id, boolean_t level);
	} stc_bsp_gpio_ops_t;

	typedef struct stc_bsp_gpio
	{
		const stc_bsp_gpio_ops_t *ops;
		uint8_t count;
		void *reserved;
	} stc_bsp_gpio_t;

	extern stc_bsp_gpio_t g_stcBspGpio;

	en_result_t Bsp_Gpio_Init(void);
	en_result_t Bsp_Gpio_InitPin(en_bsp_gpio_id_t id);
	boolean_t Bsp_Gpio_Read(en_bsp_gpio_id_t id);
	en_result_t Bsp_Gpio_Write(en_bsp_gpio_id_t id, boolean_t level);

#ifdef __cplusplus
}
#endif

#endif
