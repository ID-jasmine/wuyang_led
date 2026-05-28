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

static const stc_bsp_gpio_pin_cfg_t s_astBspGpioPinCfg[BspGpioIdCount] = {
	[BspGpioIdIgn] = {.port = GpioPortB,
					  .pin = GpioPin11,
					  .dir = BspGpioDirIn,
					  .pu = GpioPuDisable,
					  .pd = GpioPdDisable,
					  .init_level = FALSE},
	[BspGpioIdPower] = {.port = GpioPortB,
						.pin = GpioPin3,
						.dir = BspGpioDirOut,
						.pu = GpioPuDisable,
						.pd = GpioPdDisable,
						.init_level = FALSE},
	[BspGpioIdLeftTurn] = {.port = GpioPortA,
						   .pin = GpioPin5,
						   .dir = BspGpioDirIn,
						   .pu = GpioPuDisable,
						   .pd = GpioPdDisable,
						   .init_level = FALSE},
	[BspGpioIdHighBeam] = {.port = GpioPortA,
						   .pin = GpioPin6,
						   .dir = BspGpioDirIn,
						   .pu = GpioPuDisable,
						   .pd = GpioPdDisable,
						   .init_level = FALSE},
	[BspGpioIdRightTurn] = {.port = GpioPortA,
							.pin = GpioPin4,
							.dir = BspGpioDirIn,
							.pu = GpioPuDisable,
							.pd = GpioPdDisable,
							.init_level = FALSE},
	[BspGpioIdPositionLamp] = {.port = GpioPortA,
							   .pin = GpioPin3,
							   .dir = BspGpioDirIn,
							   .pu = GpioPuDisable,
							   .pd = GpioPdDisable,
							   .init_level = FALSE},
	[BspGpioIdPhotoDetec] = {.port = GpioPortB,
							 .pin = GpioPin12,
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
};

static en_result_t BspGpio_CheckId(en_bsp_gpio_id_t id)
{
	if (id >= BspGpioIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
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

static const stc_bsp_gpio_ops_t s_stcBspGpioOps = {
	.init = BspGpio_InitImpl,
	.read = BspGpio_ReadImpl,
	.write = BspGpio_WriteImpl,
};

stc_bsp_gpio_t g_stcBspGpio = {
	.ops = &s_stcBspGpioOps,
	.count = BspGpioIdCount,
	.reserved = NULL,
};

en_result_t Bsp_Gpio_Init(void)
{
	uint8_t i;
	en_result_t enRet;

	for (i = 0u; i < g_stcBspGpio.count; i++)
	{
		enRet = g_stcBspGpio.ops->init((en_bsp_gpio_id_t)i);
		if (Ok != enRet)
		{
			return enRet;
		}
	}

	return Ok;
}

en_result_t Bsp_Gpio_InitPin(en_bsp_gpio_id_t id)
{
	return g_stcBspGpio.ops->init(id);
}

boolean_t Bsp_Gpio_Read(en_bsp_gpio_id_t id)
{
	return g_stcBspGpio.ops->read(id);
}

en_result_t Bsp_Gpio_Write(en_bsp_gpio_id_t id, boolean_t level)
{
	return g_stcBspGpio.ops->write(id, level);
}
