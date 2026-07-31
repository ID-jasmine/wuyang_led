#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "adc.h"
#include "base_types.h"
#include "bt.h"
#include "gpio.h"
#include "product_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* 引脚运行态由哪个模块负责初始化。 */
typedef enum
{
	BoardPinModeGpio = 0u,
	BoardPinModeAnalog,
	BoardPinModePeripheral,
} en_board_pin_mode_t;

/* 进入深度休眠时对引脚采取的动作。 */
typedef enum
{
	BoardPinSleepKeep = 0u,
	BoardPinSleepAnalog,
	BoardPinSleepDefaultOutput,
} en_board_pin_sleep_mode_t;

/*
 * 整块板唯一的物理引脚清单。
 * 新增或换引脚时，物理端口/编号只在 board_config.c 中修改。
 */
typedef enum
{
	BoardPinIdPower = 0u,
	BoardPinIdPositionLamp,
	BoardPinIdSwK1,
	BoardPinIdSwK2,
	BoardPinIdGearN,
	BoardPinIdGear1,
	BoardPinIdGear2,
	BoardPinIdGear3,
	BoardPinIdGear4,
	BoardPinIdGear5,
	BoardPinIdGear6,
	BoardPinIdEngineFault,
	BoardPinIdLedPower,
	BoardPinIdEepromScl,
	BoardPinIdEepromSda,
	BoardPinIdNcPb8,
	BoardPinIdNcPb5,
	BoardPinIdNcPb4,
	BoardPinIdNcPa15,
	BoardPinIdNcPa12,
	BoardPinIdNcPa11,
	BoardPinIdAdcFuel,
	BoardPinIdAdcPower,
	BoardPinIdAdcWaterTemp,
	BoardPinIdAdcIgn,
	BoardPinIdAdcBrightness,
	BoardPinIdAdcLeftTurn,
	BoardPinIdAdcHighBeam,
	BoardPinIdAdcRightTurn,
	BoardPinIdCaptureSpeed,
	BoardPinIdCaptureRpm,
	BoardPinIdTm3100Sdi,
	BoardPinIdTm3100Clk,
	BoardPinIdTm3100Le,
	BoardPinIdTm3100Oe,
	BoardPinIdCount,
} en_board_pin_id_t;

typedef struct
{
	boolean_t enabled;
	en_gpio_port_t port;
	en_gpio_pin_t pin;
	en_board_pin_mode_t run_mode;
	en_board_pin_sleep_mode_t sleep_mode;
	en_gpio_dir_t dir;
	en_gpio_pu_t pull_up;
	en_gpio_pd_t pull_down;
	en_gpio_od_t open_drain;
	boolean_t init_level;
	en_gpio_af_t af;
} stc_board_pin_cfg_t;

typedef enum
{
	BoardAdcIdFuel = 0u,
	BoardAdcIdPower,
	BoardAdcIdWaterTemp,
	BoardAdcIdIgn,
	BoardAdcIdBrightness,
	BoardAdcIdLeftTurn,
	BoardAdcIdHighBeam,
	BoardAdcIdRightTurn,
	BoardAdcIdCount,
} en_board_adc_id_t;

typedef struct
{
	boolean_t enabled;
	en_board_pin_id_t pin_id;
	en_adc_samp_ch_sel_t channel;
} stc_board_adc_cfg_t;

typedef enum
{
	BoardIicBusIdEeprom = 0u,
	BoardIicBusIdCount,
} en_board_iic_bus_id_t;

typedef struct
{
	boolean_t enabled;
	en_board_pin_id_t scl_pin_id;
	en_board_pin_id_t sda_pin_id;
	uint32_t target_hz;
} stc_board_iic_cfg_t;

typedef enum
{
	BoardCaptureChannel0A = 0u,
	BoardCaptureChannel0B,
	BoardCaptureChannel1A,
	BoardCaptureChannel1B,
	BoardCaptureChannel2A,
	BoardCaptureChannel2B,
} en_board_capture_channel_t;

typedef enum
{
	BoardCaptureIdSpeed = 0u,
	BoardCaptureIdRpm,
	BoardCaptureIdCount,
} en_board_capture_id_t;

typedef struct
{
	boolean_t enabled;
	en_board_pin_id_t pin_id;
	en_board_capture_channel_t channel;
} stc_board_capture_cfg_t;

typedef struct
{
	boolean_t enabled;
	en_board_pin_id_t sdi_pin_id;
	en_board_pin_id_t clk_pin_id;
	en_board_pin_id_t le_pin_id;
	en_board_pin_id_t oe_pin_id;
	en_bt_unit_t oe_timer;
	uint16_t oe_period_ticks;
	uint8_t chip_count;
} stc_board_tm3100_cfg_t;

#define BOARD_CPU_CLOCK_HZ            (16000000u)
#define BOARD_CAPTURE_TIMER_CLOCK_HZ  (250000u)
#define BOARD_TM3100_CHIP_COUNT       (15u)
#define BOARD_TM3100_CHANNELS_PER_CHIP (16u)

extern const stc_board_pin_cfg_t g_astBoardPinCfg[BoardPinIdCount];
extern const stc_board_adc_cfg_t g_astBoardAdcCfg[BoardAdcIdCount];
extern const stc_board_iic_cfg_t g_astBoardIicCfg[BoardIicBusIdCount];
extern const stc_board_capture_cfg_t g_astBoardCaptureCfg[BoardCaptureIdCount];
extern const stc_board_tm3100_cfg_t g_stcBoardTm3100Cfg;

const stc_board_pin_cfg_t *Board_GetPinConfig(en_board_pin_id_t id);
en_result_t Board_ConfigValidate(void);

#ifdef __cplusplus
}
#endif

#endif
