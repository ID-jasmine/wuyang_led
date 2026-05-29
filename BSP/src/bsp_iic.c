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

/**
 * @brief  初始化指定的IIC总线相关引脚和配置
 * @param  [in] bus_id IIC总线的ID标识 (参考 en_bsp_iic_bus_id_t 枚举)
 * @retval Ok: 初始化成功
 * @retval Error: 初始化失败 (如传递了不支持的ID)
 */
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

/**
 * @brief  获取指定IIC总线的操作句柄
 * @param  [in] bus_id IIC总线的ID标识 (参考 en_bsp_iic_bus_id_t 枚举)
 * @return 匹配的 IIC_Handle_t 句柄指针，若失败或越界则返回 NULL
 */
IIC_Handle_t *BSP_IIC_GetHandle(en_bsp_iic_bus_id_t bus_id)
{
	if (Ok != BspIic_CheckId(bus_id))
	{
		return NULL;
	}

	return &s_astBspIicHandle[bus_id];
}

/**
 * @brief  产生IIC起始信号 (Start Condition)
 * @param  [in] iic IIC操作句柄指针
 */
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

/**
 * @brief  产生IIC停止信号 (Stop Condition)
 * @param  [in] iic IIC操作句柄指针
 */
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

/**
 * @brief  向IIC总线发送一个字节的数据
 * @param  [in] iic IIC操作句柄指针
 * @param  [in] data 待发送的8位数据
 */
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

/**
 * @brief  等待接收设备的应答信号 (ACK/NACK)
 * @param  [in] iic IIC操作句柄指针
 */
en_result_t BSP_IIC_WaitAck(IIC_Handle_t *iic)
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
			return Error;
		}
	}
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);

	return Ok;
}

/**
 * @brief  从IIC总线读取一个字节的数据，并根据参数发送应答或非应答信号
 * @param  [in] iic IIC操作句柄指针
 * @param  [in] ack 读取完成后发送的应答状态 (非0为ACK, 0为NACK)
 * @return 读取到的8位数据
 */
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
