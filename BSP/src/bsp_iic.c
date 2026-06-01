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
	BspIic_Delay(4u);
	(void)Gpio_ClrIO(iic->sda_port, iic->sda_pin);
	BspIic_Delay(4u);
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
	BspIic_Delay(4u);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(4u);
	(void)Gpio_SetIO(iic->sda_port, iic->sda_pin);
	BspIic_Delay(4u);
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
		BspIic_Delay(4u);
		(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
		BspIic_Delay(4u);
		(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);
		BspIic_Delay(4u);
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
	BspIic_Delay(2u);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(2u);

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
		BspIic_Delay(4u);
		(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
		receive <<= 1;
		if (Gpio_GetInputIO(iic->sda_port, iic->sda_pin))
		{
			receive++;
		}
		BspIic_Delay(4u);
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
	BspIic_Delay(4u);
	(void)Gpio_SetIO(iic->scl_port, iic->scl_pin);
	BspIic_Delay(4u);
	(void)Gpio_ClrIO(iic->scl_port, iic->scl_pin);

	return receive;
}

// =========================================================================
// EEPROM IIC 通信底层 (PB6=SCL, PB7=SDA)
// =========================================================================
void EEPROM_IIC_Init(void) {
	stc_gpio_cfg_t stcGpioCfg;
	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)Gpio_Init(GpioPortB, GpioPin6, &stcGpioCfg); // SCL
	(void)Gpio_Init(GpioPortB, GpioPin7, &stcGpioCfg); // SDA

	EE_SCL_H();
	EE_SDA_H();
}

static void EE_SDA_Dir_In(void) {
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirIn;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)Gpio_Init(GpioPortB, GpioPin7, &stcGpioCfg);
}

static void EE_SDA_Dir_Out(void) {
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)Gpio_Init(GpioPortB, GpioPin7, &stcGpioCfg);
}

void EE_IIC_Start(void) {
	EE_SDA_Dir_Out();
	EE_SDA_H();
	EE_SCL_H();
	BspIic_Delay(4u);
	EE_SDA_L();
	BspIic_Delay(4u);
	EE_SCL_L();
}

void EE_IIC_Stop(void) {
	EE_SDA_Dir_Out();
	EE_SCL_L();
	EE_SDA_L();
	BspIic_Delay(4u);
	EE_SCL_H();
	BspIic_Delay(4u);
	EE_SDA_H();
	BspIic_Delay(4u);
}

uint8_t EE_IIC_Wait_Ack(void) {
	uint8_t errTime = 0u;

	EE_SDA_Dir_In();
	EE_SDA_H();
	BspIic_Delay(2u);
	EE_SCL_H();
	BspIic_Delay(2u);

	while (EE_SDA_IN()) {
		errTime++;
		if (errTime > 250u) {
			EE_IIC_Stop();
			return 1u;
		}
	}
	EE_SCL_L();
	return 0u;
}

void EE_IIC_Send(uint8_t data) {
	uint8_t i;

	EE_SDA_Dir_Out();
	EE_SCL_L();
	for (i = 0u; i < 8u; i++) {
		if (data & 0x80u)
			EE_SDA_H();
		else
			EE_SDA_L();
		data <<= 1;
		BspIic_Delay(4u);
		EE_SCL_H();
		BspIic_Delay(4u);
		EE_SCL_L();
		BspIic_Delay(4u);
	}
}

uint8_t EE_IIC_ReadByte(uint8_t ack) {
	uint8_t i, receive = 0u;

	EE_SDA_Dir_In();
	for (i = 0u; i < 8u; i++) {
		EE_SCL_L();
		BspIic_Delay(4u);
		EE_SCL_H();
		receive <<= 1;
		if (EE_SDA_IN())
			receive++;
		BspIic_Delay(4u);
	}

	EE_SDA_Dir_Out();
	EE_SCL_L();
	if (!ack)
		EE_SDA_L();
	else
		EE_SDA_H();
	BspIic_Delay(4u);
	EE_SCL_H();
	BspIic_Delay(4u);
	EE_SCL_L();

	return receive;
}
