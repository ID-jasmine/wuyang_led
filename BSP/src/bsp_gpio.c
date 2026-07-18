#include "bsp_gpio.h"
#include "gpio.h"
#include "sysctrl.h"

static en_result_t BspGpio_CheckId(en_bsp_gpio_id_t id)
{
	const stc_board_pin_cfg_t *pstcPinCfg;

	pstcPinCfg = Board_GetPinConfig(id);
	if ((NULL == pstcPinCfg) || (FALSE == pstcPinCfg->enabled))
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static en_result_t BspGpio_SetAnalogModeById(en_bsp_gpio_id_t id)
{
	const stc_board_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcPinCfg = Board_GetPinConfig(id);
	return Gpio_SetAnalogMode(pstcPinCfg->port, pstcPinCfg->pin);
}

static en_result_t BspGpio_SetDefaultOutputById(en_bsp_gpio_id_t id)
{
	const stc_board_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcPinCfg = Board_GetPinConfig(id);
	if ((BoardPinModeGpio != pstcPinCfg->run_mode) || (GpioDirOut != pstcPinCfg->dir))
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
	const stc_board_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	pstcPinCfg = Board_GetPinConfig(id);
	if (NULL == pstcPinCfg)
	{
		return ErrorInvalidParameter;
	}
	if (FALSE == pstcPinCfg->enabled)
	{
		return Ok;
	}

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	if (BoardPinModePeripheral == pstcPinCfg->run_mode)
	{
		return Ok;
	}
	if (BoardPinModeAnalog == pstcPinCfg->run_mode)
	{
		return Gpio_SetAnalogMode(pstcPinCfg->port, pstcPinCfg->pin);
	}

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = pstcPinCfg->dir;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = pstcPinCfg->pull_up;
	stcGpioCfg.enPd = pstcPinCfg->pull_down;
	stcGpioCfg.enOD = pstcPinCfg->open_drain;
	stcGpioCfg.enCtrlMode = GpioFastIO;

	enRet = Gpio_Init(pstcPinCfg->port, pstcPinCfg->pin, &stcGpioCfg);
	if (Ok != enRet)
	{
		return enRet;
	}

	if (GpioDirOut == pstcPinCfg->dir)
	{
		(void)Bsp_Gpio_Write(id, pstcPinCfg->init_level);
	}

	return Ok;
}

static boolean_t BspGpio_ReadImpl(en_bsp_gpio_id_t id)
{
	const stc_board_pin_cfg_t *pstcPinCfg;

	if (Ok != BspGpio_CheckId(id))
	{
		return FALSE;
	}

	pstcPinCfg = Board_GetPinConfig(id);
	if (BoardPinModeGpio != pstcPinCfg->run_mode)
	{
		return FALSE;
	}

	return Gpio_GetInputIO(pstcPinCfg->port, pstcPinCfg->pin);
}

static en_result_t BspGpio_WriteImpl(en_bsp_gpio_id_t id, boolean_t level)
{
	const stc_board_pin_cfg_t *pstcPinCfg;
	en_result_t enRet;

	enRet = BspGpio_CheckId(id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcPinCfg = Board_GetPinConfig(id);
	if ((BoardPinModeGpio != pstcPinCfg->run_mode) || (GpioDirOut != pstcPinCfg->dir))
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
	return Board_ConfigValidate();
}

static en_result_t BspGpio_InitSleepPinsImpl(void)
{
	uint8_t i;
	en_result_t enRet;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	for (i = 0u; i < BoardPinIdCount; i++)
	{
		if (FALSE == g_astBoardPinCfg[i].enabled)
		{
			continue;
		}

		switch (g_astBoardPinCfg[i].sleep_mode)
		{
		case BoardPinSleepAnalog:
			enRet = BspGpio_SetAnalogModeById((en_bsp_gpio_id_t)i);
			break;

		case BoardPinSleepDefaultOutput:
			enRet = BspGpio_SetDefaultOutputById((en_bsp_gpio_id_t)i);
			break;

		case BoardPinSleepKeep:
		default:
			enRet = Ok;
			break;
		}

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

	enRet = Board_ConfigValidate();
	if (Ok != enRet)
	{
		return enRet;
	}

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
