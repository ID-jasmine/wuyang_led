#include "bsp_iic.h"

static IIC_Handle_t s_astBspIicHandle[BspIicBusIdCount] = {
	[BspIicBusIdEeprom] = {.scl_port = GpioPortB,
						   .scl_pin = GpioPin6,
						   .sda_port = GpioPortB,
						   .sda_pin = GpioPin7},
};

static void BspIic_Delay(volatile uint32_t time)
{
	while (time-- > 0u)
	{
	}
}

static en_result_t BspIic_CheckId(en_bsp_iic_bus_id_t bus_id)
{
	if (bus_id >= BspIicBusIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static void BspIic_SdaDirIn(IIC_Handle_t *iic)
{
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirIn;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)Gpio_Init(iic->sda_port, iic->sda_pin, &stcGpioCfg);
}

static void BspIic_SdaDirOut(IIC_Handle_t *iic)
{
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)Gpio_Init(iic->sda_port, iic->sda_pin, &stcGpioCfg);
}

static void BspIic_GpioInit(IIC_Handle_t *iic)
{
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)Gpio_Init(iic->sda_port, iic->sda_pin, &stcGpioCfg);
	(void)Gpio_Init(iic->scl_port, iic->scl_pin, &stcGpioCfg);
	(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
}

en_result_t BSP_IIC_InitBus(en_bsp_iic_bus_id_t bus_id)
{
	en_result_t enRet;

	enRet = BspIic_CheckId(bus_id);
	if (Ok != enRet)
	{
		return enRet;
	}

	BspIic_GpioInit(&s_astBspIicHandle[bus_id]);
	return Ok;
}

IIC_Handle_t *BSP_IIC_GetHandle(en_bsp_iic_bus_id_t bus_id)
{
	if (Ok != BspIic_CheckId(bus_id))
	{
		return NULL;
	}

	return &s_astBspIicHandle[bus_id];
}

void BSP_IIC_Start(IIC_Handle_t *iic)
{
	BspIic_SdaDirOut(iic);
	(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(1u);
	(void)Gpio_ClrIO(iic->sda_port, iic->sda_pin);
	BspIic_Delay(1u);
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
}

void BSP_IIC_Stop(IIC_Handle_t *iic)
{
	BspIic_SdaDirOut(iic);
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
	(void)Gpio_ClrIO(iic->sda_port, iic->sda_pin);
	BspIic_Delay(1u);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(1u);
	(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
	BspIic_Delay(1u);
}

void BSP_IIC_Send(IIC_Handle_t *iic, uint8_t data)
{
	uint8_t i;

	BspIic_SdaDirOut(iic);
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
	for (i = 0u; i < 8u; i++)
	{
		if (0u != (data & 0x80u))
		{
			(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
		}
		else
		{
			(void)Gpio_ClrIO(iic->sda_port, iic->sda_pin);
		}
		data <<= 1;
		BspIic_Delay(1u);
		(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
		BspIic_Delay(1u);
		(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
		BspIic_Delay(1u);
	}
}

void BSP_IIC_WaitAck(IIC_Handle_t *iic)
{
	uint8_t errTime = 0u;

	BspIic_SdaDirIn(iic);
	(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
	BspIic_Delay(1u);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(1u);

	while (Gpio_GetInputIO(iic->sda_port, iic->sda_pin))
	{
		errTime++;
		if (errTime > 250u)
		{
			BSP_IIC_Stop(iic);
			break;
		}
	}
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
}

uint8_t BSP_IIC_ReadByte(IIC_Handle_t *iic, uint8_t ack)
{
	uint8_t i;
	uint8_t receive = 0u;

	BspIic_SdaDirIn(iic);
	for (i = 0u; i < 8u; i++)
	{
		(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
		BspIic_Delay(1u);
		(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
		receive <<= 1;
		if (Gpio_GetInputIO(iic->sda_port, iic->sda_pin))
		{
			receive++;
		}
		BspIic_Delay(1u);
	}

	BspIic_SdaDirOut(iic);
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
	if (0u == ack)
	{
		(void)Gpio_ClrIO(iic->sda_port, iic->sda_pin);
	}
	else
	{
		(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
	}
	BspIic_Delay(1u);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(1u);
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);

	return receive;
}
