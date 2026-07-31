#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include "board_config.h"
#include "ddl.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef en_board_pin_id_t en_bsp_gpio_id_t;

/* 兼容现有调用；物理定义以 BOARD 的逻辑引脚 ID 为准。 */
#define BspGpioIdPower        BoardPinIdPower
#define BspGpioIdPositionLamp BoardPinIdPositionLamp
#define BspGpioIdSwK1         BoardPinIdSwK1
#define BspGpioIdSwK2         BoardPinIdSwK2
#define BspGpioIdGearN        BoardPinIdGearN
#define BspGpioIdGear1        BoardPinIdGear1
#define BspGpioIdGear2        BoardPinIdGear2
#define BspGpioIdGear3        BoardPinIdGear3
#define BspGpioIdGear4        BoardPinIdGear4
#define BspGpioIdGear5        BoardPinIdGear5
#define BspGpioIdGear6        BoardPinIdGear6
#define BspGpioIdEnginefault  BoardPinIdEngineFault
#define BspGpioIdLEDPower     BoardPinIdLedPower
#define BspGpioIdEepromScl    BoardPinIdEepromScl
#define BspGpioIdEepromSda    BoardPinIdEepromSda
#define BspGpioIdCount        BoardPinIdCount

	typedef enum
	{
		BspGpioDirOut = 0u,
		BspGpioDirIn = 1u,
	} en_bsp_gpio_dir_t;

	typedef struct
	{
		en_result_t (*init)(en_bsp_gpio_id_t id);
		en_result_t (*init_nc_pins)(void);
		en_result_t (*init_sleep_pins)(void);
		boolean_t (*read)(en_bsp_gpio_id_t id);
		en_result_t (*write)(en_bsp_gpio_id_t id, boolean_t level);
	} stc_bsp_gpio_ops_t;

	typedef struct
	{
		const stc_bsp_gpio_ops_t *ops;
		uint8_t count;
		void *reserved;
	} stc_bsp_gpio_t;

	extern stc_bsp_gpio_t g_stcBspGpio;

	en_result_t Bsp_Gpio_Init(void);
	en_result_t Bsp_Gpio_InitSleepPins(void);
	boolean_t Bsp_Gpio_Read(en_bsp_gpio_id_t id);
	en_result_t Bsp_Gpio_Write(en_bsp_gpio_id_t id, boolean_t level);

#ifdef __cplusplus
}
#endif

#endif
