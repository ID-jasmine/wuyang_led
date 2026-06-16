#include "bsp_gpio.h"
#include "gpio.h"
#include "sysctrl.h"

typedef struct stc_bsp_gpio_pin_cfg
{
	en_gpio_port_t port;
	en_gpio_pin_t pin;
	en_bsp_gpio_dir_t dir;
	en_gpio_pu_t pu;
	en_gpio_pd_t pd;
	boolean_t init_level;
} stc_bsp_gpio_pin_cfg_t;

typedef struct stc_bsp_gpio_nc_pin_cfg
{
	en_gpio_port_t port;
	en_gpio_pin_t pin;
} stc_bsp_gpio_nc_pin_cfg_t;

static const stc_bsp_gpio_pin_cfg_t s_astBspGpioPinCfg[BspGpioIdCount] = {
	[BspGpioIdPower] = {.port = GpioPortB,
						.pin = GpioPin3,
						.dir = BspGpioDirOut,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdPositionLamp] = {.port = GpioPortA,
							   .pin = GpioPin3,
							   .dir = BspGpioDirIn,
							   .pu = GpioPuDisable,
							   .pd = GpioPdDisable,
							   .init_level = FALSE},
	[BspGpioIdSwK1] = {.port = GpioPortD,
					   .pin = GpioPin6,
					   .dir = BspGpioDirIn,
					   .pu = GpioPuEnable,
					   .pd = GpioPdDisable,
					   .init_level = FALSE},
	[BspGpioIdSwK2] = {.port = GpioPortD,
					   .pin = GpioPin7,
					   .dir = BspGpioDirIn,
					   .pu = GpioPuEnable,
					   .pd = GpioPdDisable,
					   .init_level = FALSE},
	[BspGpioIdGearN] = {.port = GpioPortB,
						.pin = GpioPin1,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdGear1] = {.port = GpioPortD,
						.pin = GpioPin1,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdGear2] = {.port = GpioPortA,
						.pin = GpioPin7,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdGear3] = {.port = GpioPortB,
						.pin = GpioPin0,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdGear4] = {.port = GpioPortB,
						.pin = GpioPin2,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdGear5] = {.port = GpioPortC,
						.pin = GpioPin13,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdGear6] = {.port = GpioPortD,
						.pin = GpioPin0,
						.dir = BspGpioDirIn,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdEnginefault] = {.port = GpioPortB,
							  .pin = GpioPin10,
							  .dir = BspGpioDirIn,
							  .pu = GpioPuDisable,
							  .pd = GpioPdDisable,
							  .init_level = FALSE},
	[BspGpioIdLEDPower] = {.port = GpioPortB,
						   .pin = GpioPin9,
						   .dir = BspGpioDirOut,
						   .pu = GpioPuDisable,
						   .pd = GpioPdDisable,
						   .init_level = FALSE},
	[BspGpioIdEepromScl] = {.port = GpioPortB,
							.pin = GpioPin6,
							.dir = BspGpioDirOut,
							.pu = GpioPuDisable,
							.pd = GpioPdDisable,
							.init_level = TRUE},
	[BspGpioIdEepromSda] = {.port = GpioPortB,
							.pin = GpioPin7,
							.dir = BspGpioDirOut,
							.pu = GpioPuDisable,
							.pd = GpioPdDisable,
							.init_level = TRUE},
};

static const stc_bsp_gpio_nc_pin_cfg_t s_astBspGpioNcPinCfg[] = {
	{GpioPortB, GpioPin8},	{GpioPortB, GpioPin5},	{GpioPortB, GpioPin4},
	{GpioPortA, GpioPin15}, {GpioPortA, GpioPin12}, {GpioPortA, GpioPin11},
};

static const stc_bsp_gpio_nc_pin_cfg_t s_astBspGpioSleepAnalogPinCfg[] = {
	{GpioPortA, GpioPin9},
	{GpioPortA, GpioPin10},
};

static const en_bsp_gpio_id_t s_aBspGpioSleepPinIds[] = {
	BspGpioIdPositionLamp, BspGpioIdSwK1,  BspGpioIdSwK2,		 BspGpioIdGearN,
	BspGpioIdGear1,		   BspGpioIdGear2, BspGpioIdGear3,		 BspGpioIdGear4,
	BspGpioIdGear5,		   BspGpioIdGear6, BspGpioIdEnginefault, BspGpioIdEepromScl,
	BspGpioIdEepromSda,
};

static const en_bsp_gpio_id_t s_aBspGpioSleepDefaultOutputIds[] = {
	BspGpioIdPower,
	BspGpioIdLEDPower,
};

enum
{
	BspGpioNcPinCount = sizeof(s_astBspGpioNcPinCfg) / sizeof(s_astBspGpioNcPinCfg[0]),
	BspGpioSleepAnalogPinCount =
		sizeof(s_astBspGpioSleepAnalogPinCfg) / sizeof(s_astBspGpioSleepAnalogPinCfg[0]),
	BspGpioSleepPinCount =
		sizeof(s_aBspGpioSleepPinIds) / sizeof(s_aBspGpioSleepPinIds[0]),
	BspGpioSleepDefaultOutputCount = sizeof(s_aBspGpioSleepDefaultOutputIds) /
									 sizeof(s_aBspGpioSleepDefaultOutputIds[0]),
};

static en_result_t BspGpio_CheckId(en_bsp_gpio_id_t id)
{
	if (id >= BspGpioIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static en_result_t BspGpio_SetAnalogModeById(en_bsp_gpio_id_t id)
{
	const stc_bsp_gpio_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcPinCfg = &s_astBspGpioPinCfg[id];
	Gpio_SetAnalogMode(pstcPinCfg->port, pstcPinCfg->pin);

	return Ok;
}

static en_result_t BspGpio_SetDefaultOutputById(en_bsp_gpio_id_t id)
{
	const stc_bsp_gpio_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcPinCfg = &s_astBspGpioPinCfg[id];
	if (BspGpioDirOut != pstcPinCfg->dir)
	{
		return ErrorInvalidParameter;
	}

	if (pstcPinCfg->init_level)
	{
		return Gpio_SetIO(pstcPinCfg->port, pstcPinCfg->pin);
	}

	return Gpio_ClrIO(pstcPinCfg->port, pstcPinCfg->pin);
}

static en_result_t BspGpio_InitImpl(en_bsp_gpio_id_t id)
{
	stc_gpio_cfg_t stcGpioCfg;
	const stc_bsp_gpio_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	pstcPinCfg = &s_astBspGpioPinCfg[id];

	stcGpioCfg.enDir = (BspGpioDirIn == pstcPinCfg->dir) ? GpioDirIn : GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = pstcPinCfg->pu;
	stcGpioCfg.enPd = pstcPinCfg->pd;
	stcGpioCfg.enOD = GpioOdDisable;
	stcGpioCfg.enCtrlMode = GpioFastIO;

	enRet = Gpio_Init(pstcPinCfg->port, pstcPinCfg->pin, &stcGpioCfg);
	if (Ok != enRet)
	{
		return enRet;
	}

	if (BspGpioDirOut == pstcPinCfg->dir)
	{
		(void)Bsp_Gpio_Write(id, pstcPinCfg->init_level);
	}

	return Ok;
}

static boolean_t BspGpio_ReadImpl(en_bsp_gpio_id_t id)
{
	const stc_bsp_gpio_pin_cfg_t *pstcPinCfg;

	if (Ok != BspGpio_CheckId(id))
	{
		return FALSE;
	}

	pstcPinCfg = &s_astBspGpioPinCfg[id];

	return Gpio_GetInputIO(pstcPinCfg->port, pstcPinCfg->pin);
}

static en_result_t BspGpio_WriteImpl(en_bsp_gpio_id_t id, boolean_t level)
{
	const stc_bsp_gpio_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcPinCfg = &s_astBspGpioPinCfg[id];
	if (BspGpioDirOut != pstcPinCfg->dir)
	{
		return ErrorInvalidParameter;
	}

	if (level)
	{
		return Gpio_SetIO(pstcPinCfg->port, pstcPinCfg->pin);
	}

	return Gpio_ClrIO(pstcPinCfg->port, pstcPinCfg->pin);
}

static en_result_t BspGpio_InitNcPinsImpl(void)
{
	uint8_t i;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	for (i = 0u; i < BspGpioNcPinCount; i++)
	{
		Gpio_SetAnalogMode(s_astBspGpioNcPinCfg[i].port, s_astBspGpioNcPinCfg[i].pin);
	}

	return Ok;
}

static en_result_t BspGpio_InitSleepPinsImpl(void)
{
	uint8_t i;
	en_result_t enRet;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	for (i = 0u; i < BspGpioSleepDefaultOutputCount; i++)
	{
		enRet = BspGpio_SetDefaultOutputById(s_aBspGpioSleepDefaultOutputIds[i]);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	for (i = 0u; i < BspGpioSleepPinCount; i++)
	{
		enRet = BspGpio_SetAnalogModeById(s_aBspGpioSleepPinIds[i]);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	for (i = 0u; i < BspGpioSleepAnalogPinCount; i++)
	{
		enRet = Gpio_SetAnalogMode(s_astBspGpioSleepAnalogPinCfg[i].port,
								   s_astBspGpioSleepAnalogPinCfg[i].pin);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

static const stc_bsp_gpio_ops_t s_stcBspGpioOps = {
	.init = BspGpio_InitImpl,
	.init_nc_pins = BspGpio_InitNcPinsImpl,
	.init_sleep_pins = BspGpio_InitSleepPinsImpl,
	.read = BspGpio_ReadImpl,
	.write = BspGpio_WriteImpl,
};

stc_bsp_gpio_t g_stcBspGpio = {
	.ops = &s_stcBspGpioOps,
	.count = BspGpioIdCount,
	.reserved = NULL,
};

/**
 * @brief  初始化BSP GPIO模块
 * @details 依次初始化所有已配置GPIO，并将未使用的NC引脚设置为模拟模式。
 * @retval Ok: 所有GPIO及NC引脚初始化成功
 * @retval Error: 存在任一GPIO或NC引脚初始化失败
 */
en_result_t Bsp_Gpio_Init(void)
{
	uint8_t i;
	en_result_t enRet;
	en_result_t enFinalRet = Ok;

	for (i = 0u; i < g_stcBspGpio.count; i++)
	{
		enRet = g_stcBspGpio.ops->init((en_bsp_gpio_id_t)i);
		if (Ok != enRet)
		{
			enFinalRet = Error;
		}
	}

	enRet = g_stcBspGpio.ops->init_nc_pins();
	if (Ok != enRet)
	{
		enFinalRet = Error;
	}

	return enFinalRet;
}

/**
 * @brief  初始化休眠模式下的GPIO配置
 * @details 将指定输入引脚切换为模拟模式，并恢复默认输出引脚的预设电平。
 * @retval Ok: 休眠态引脚配置成功
 * @retval ErrorInvalidParameter: 引脚ID无效或输出配置不匹配
 * @retval 其他: 底层GPIO操作失败
 */
en_result_t Bsp_Gpio_InitSleepPins(void)
{
	return g_stcBspGpio.ops->init_sleep_pins();
}

/**
 * @brief  读取指定GPIO电平
 * @param  [in] id GPIO功能ID，参见 en_bsp_gpio_id_t
 * @return TRUE表示高电平，FALSE表示低电平或参数无效
 */
boolean_t Bsp_Gpio_Read(en_bsp_gpio_id_t id)
{
	return g_stcBspGpio.ops->read(id);
}

/**
 * @brief  设置指定GPIO输出电平
 * @param  [in] id GPIO功能ID，参见 en_bsp_gpio_id_t
 * @param  [in] level 输出电平，TRUE为高，FALSE为低
 * @retval Ok: 设置成功
 * @retval ErrorInvalidParameter: 引脚ID无效或该引脚不是输出模式
 * @retval 其他: 底层GPIO写操作失败
 */
en_result_t Bsp_Gpio_Write(en_bsp_gpio_id_t id, boolean_t level)
{
	return g_stcBspGpio.ops->write(id, level);
}
