#include "bsp_iic.h"

#include "gpio.h"

#define BSP_IIC_DELAY_LOOP_CYCLES  (20u)
#define BSP_IIC_MIN_DELAY_LOOPS	   (4u)
#define BSP_IIC_MIN_ACK_DELAY_LOOPS (2u)

typedef struct stc_bsp_iic_gpio_ops
{
	en_result_t (*init)(en_gpio_port_t port, en_gpio_pin_t pin, stc_gpio_cfg_t *cfg);
	void (*set)(en_gpio_port_t port, en_gpio_pin_t pin);
	void (*clr)(en_gpio_port_t port, en_gpio_pin_t pin);
	boolean_t (*read)(en_gpio_port_t port, en_gpio_pin_t pin);
} stc_bsp_iic_gpio_ops_t;

typedef struct stc_bsp_iic_pin
{
	en_gpio_port_t port;
	en_gpio_pin_t pin;
} stc_bsp_iic_pin_t;

struct stc_bsp_iic_handle
{
	stc_bsp_iic_pin_t scl;
	stc_bsp_iic_pin_t sda;
	const stc_bsp_iic_gpio_ops_t *gpio;
	uint32_t delay_loops;
	uint32_t ack_delay_loops;
	boolean_t initialized;
};

static void BspIic_GpioSet(en_gpio_port_t port, en_gpio_pin_t pin)
{
	(void)Gpio_SetIO(port, pin);
}

static void BspIic_GpioClr(en_gpio_port_t port, en_gpio_pin_t pin)
{
	(void)Gpio_ClrIO(port, pin);
}

static const stc_bsp_iic_gpio_ops_t s_stcBspIicGpioOps = {
	.init = Gpio_Init,
	.set = BspIic_GpioSet,
	.clr = BspIic_GpioClr,
	.read = Gpio_GetInputIO,
};

static struct stc_bsp_iic_handle s_astBspIicHandle[BspIicBusIdCount];

static void BspIic_Delay(volatile uint32_t loops)
{
	while (loops-- > 0u)
	{
	}
}

static uint32_t BspIic_CalcDelayLoops(uint32_t target_hz)
{
	uint32_t loops;

	if (0u == target_hz)
	{
		target_hz = 100000u;
	}

	loops = BOARD_CPU_CLOCK_HZ / (target_hz * 2u * BSP_IIC_DELAY_LOOP_CYCLES);
	if (loops < BSP_IIC_MIN_DELAY_LOOPS)
	{
		loops = BSP_IIC_MIN_DELAY_LOOPS;
	}

	return loops;
}

static en_result_t BspIic_CheckId(en_bsp_iic_bus_id_t bus_id)
{
	if (bus_id >= BspIicBusIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static en_result_t BspIic_CheckHandle(IIC_Handle_t *iic)
{
	if ((NULL == iic) || (NULL == iic->gpio))
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

static void BspIic_SclHigh(IIC_Handle_t *iic)
{
	iic->gpio->set(iic->scl.port, iic->scl.pin);
}

static void BspIic_SclLow(IIC_Handle_t *iic)
{
	iic->gpio->clr(iic->scl.port, iic->scl.pin);
}

static void BspIic_SdaHigh(IIC_Handle_t *iic)
{
	iic->gpio->set(iic->sda.port, iic->sda.pin);
}

static void BspIic_SdaLow(IIC_Handle_t *iic)
{
	iic->gpio->clr(iic->sda.port, iic->sda.pin);
}

static boolean_t BspIic_ReadSda(IIC_Handle_t *iic)
{
	return iic->gpio->read(iic->sda.port, iic->sda.pin);
}

static void BspIic_SdaDirIn(IIC_Handle_t *iic)
{
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirIn;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)iic->gpio->init(iic->sda.port, iic->sda.pin, &stcGpioCfg);
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

	(void)iic->gpio->init(iic->sda.port, iic->sda.pin, &stcGpioCfg);
}

static void BspIic_PinDirOut(IIC_Handle_t *iic, const stc_bsp_iic_pin_t *pin)
{
	stc_gpio_cfg_t stcGpioCfg;

	DDL_ZERO_STRUCT(stcGpioCfg);
	stcGpioCfg.enDir = GpioDirOut;
	stcGpioCfg.enDrv = GpioDrvH;
	stcGpioCfg.enPu = GpioPuDisable;
	stcGpioCfg.enPd = GpioPdDisable;
	stcGpioCfg.enOD = GpioOdEnable;

	(void)iic->gpio->init(pin->port, pin->pin, &stcGpioCfg);
}

static en_result_t BspIic_GpioInit(IIC_Handle_t *iic, uint32_t target_hz)
{
	if (Ok != BspIic_CheckHandle(iic))
	{
		return ErrorInvalidParameter;
	}

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
	iic->delay_loops = BspIic_CalcDelayLoops(target_hz);
	iic->ack_delay_loops = iic->delay_loops / 2u;
	if (iic->ack_delay_loops < BSP_IIC_MIN_ACK_DELAY_LOOPS)
	{
		iic->ack_delay_loops = BSP_IIC_MIN_ACK_DELAY_LOOPS;
	}

	BspIic_PinDirOut(iic, &iic->sda);
	BspIic_PinDirOut(iic, &iic->scl);
	BspIic_SdaHigh(iic);
	BspIic_SclHigh(iic);
	iic->initialized = TRUE;

	return Ok;
}

en_result_t BSP_IIC_InitBus(en_bsp_iic_bus_id_t bus_id)
{
	en_result_t enRet;
	IIC_Handle_t *iic;
	const stc_board_iic_cfg_t *pstcBusCfg;
	const stc_board_pin_cfg_t *pstcSclPinCfg;
	const stc_board_pin_cfg_t *pstcSdaPinCfg;

	enRet = BspIic_CheckId(bus_id);
	if (Ok != enRet)
	{
		return enRet;
	}

	pstcBusCfg = &g_astBoardIicCfg[bus_id];
	if (FALSE == pstcBusCfg->enabled)
	{
		return ErrorUninitialized;
	}

	pstcSclPinCfg = Board_GetPinConfig(pstcBusCfg->scl_pin_id);
	pstcSdaPinCfg = Board_GetPinConfig(pstcBusCfg->sda_pin_id);
	if ((NULL == pstcSclPinCfg) || (NULL == pstcSdaPinCfg))
	{
		return ErrorInvalidParameter;
	}

	iic = &s_astBspIicHandle[bus_id];
	iic->gpio = &s_stcBspIicGpioOps;
	iic->scl.port = pstcSclPinCfg->port;
	iic->scl.pin = pstcSclPinCfg->pin;
	iic->sda.port = pstcSdaPinCfg->port;
	iic->sda.pin = pstcSdaPinCfg->pin;

	return BspIic_GpioInit(iic, pstcBusCfg->target_hz);
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
	if (Ok != BspIic_CheckHandle(iic))
	{
		return;
	}

	BspIic_SdaDirOut(iic);
	BspIic_SdaHigh(iic);
	BspIic_SclHigh(iic);
	BspIic_Delay(iic->delay_loops);
	BspIic_SdaLow(iic);
	BspIic_Delay(iic->delay_loops);
	BspIic_SclLow(iic);
}

void BSP_IIC_Stop(IIC_Handle_t *iic)
{
	if (Ok != BspIic_CheckHandle(iic))
	{
		return;
	}

	BspIic_SdaDirOut(iic);
	BspIic_SclLow(iic);
	BspIic_SdaLow(iic);
	BspIic_Delay(iic->delay_loops);
	BspIic_SclHigh(iic);
	BspIic_Delay(iic->delay_loops);
	BspIic_SdaHigh(iic);
	BspIic_Delay(iic->delay_loops);
}

void BSP_IIC_Send(IIC_Handle_t *iic, uint8_t data)
{
	uint8_t i;

	if (Ok != BspIic_CheckHandle(iic))
	{
		return;
	}

	BspIic_SdaDirOut(iic);
	BspIic_SclLow(iic);
	for (i = 0u; i < 8u; i++)
	{
		if (0u != (data & 0x80u))
		{
			BspIic_SdaHigh(iic);
		}
		else
		{
			BspIic_SdaLow(iic);
		}

		data <<= 1;
		BspIic_Delay(iic->delay_loops);
		BspIic_SclHigh(iic);
		BspIic_Delay(iic->delay_loops);
		BspIic_SclLow(iic);
		BspIic_Delay(iic->delay_loops);
	}
}

en_result_t BSP_IIC_WaitAck(IIC_Handle_t *iic)
{
	uint8_t errTime = 0u;

	if (Ok != BspIic_CheckHandle(iic))
	{
		return ErrorInvalidParameter;
	}

	BspIic_SdaHigh(iic);
	BspIic_SdaDirIn(iic);
	BspIic_Delay(iic->ack_delay_loops);
	BspIic_SclHigh(iic);
	BspIic_Delay(iic->ack_delay_loops);

	while (TRUE == BspIic_ReadSda(iic))
	{
		errTime++;
		if (errTime > 250u)
		{
			BSP_IIC_Stop(iic);
			return Error;
		}
	}

	BspIic_SclLow(iic);
	return Ok;
}

uint8_t BSP_IIC_ReadByte(IIC_Handle_t *iic, en_bsp_iic_ack_t ack)
{
	uint8_t i;
	uint8_t receive = 0u;

	if (Ok != BspIic_CheckHandle(iic))
	{
		return 0u;
	}

	BspIic_SdaDirIn(iic);
	for (i = 0u; i < 8u; i++)
	{
		BspIic_SclLow(iic);
		BspIic_Delay(iic->delay_loops);
		BspIic_SclHigh(iic);
		receive <<= 1;
		if (TRUE == BspIic_ReadSda(iic))
		{
			receive++;
		}
		BspIic_Delay(iic->delay_loops);
	}

	BspIic_SdaDirOut(iic);
	BspIic_SclLow(iic);
	if (BspIicAck == ack)
	{
		BspIic_SdaLow(iic);
	}
	else
	{
		BspIic_SdaHigh(iic);
	}
	BspIic_Delay(iic->delay_loops);
	BspIic_SclHigh(iic);
	BspIic_Delay(iic->delay_loops);
	BspIic_SclLow(iic);

	return receive;
}
