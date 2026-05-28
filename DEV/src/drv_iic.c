#include "drv_iic.h"

static en_result_t DrvIic_HwInit(stc_drv_iic_bus_t *bus);
static void DrvIic_HwStart(stc_drv_iic_bus_t *bus);
static void DrvIic_HwStop(stc_drv_iic_bus_t *bus);
static void DrvIic_HwSend(stc_drv_iic_bus_t *bus, uint8_t data);
static void DrvIic_HwWaitAck(stc_drv_iic_bus_t *bus);
static uint8_t DrvIic_HwReadByte(stc_drv_iic_bus_t *bus, uint8_t ack);
static void DrvIic_ApplyDefaultConfig(void);

static const stc_drv_iic_bus_ops_t s_stcDrvIicBusOps = {
	.init = DrvIic_HwInit,
	.start = DrvIic_HwStart,
	.stop = DrvIic_HwStop,
	.send = DrvIic_HwSend,
	.wait_ack = DrvIic_HwWaitAck,
	.read_byte = DrvIic_HwReadByte,
};

static stc_drv_iic_bus_t s_astDrvIicBus[DrvIicBusIdCount];
static boolean_t s_bDrvIicBusCfgDone = FALSE;

static en_result_t DrvIic_CheckId(en_drv_iic_bus_id_t bus_id)
{
	if (bus_id >= DrvIicBusIdCount)
	{
		return ErrorInvalidParameter;
	}

	return Ok;
}

en_result_t DRV_IIC_Bus_Init(stc_drv_iic_bus_t *bus)
{
	if ((NULL == bus) || (NULL == bus->ops) || (NULL == bus->ops->init))
	{
		return ErrorInvalidParameter;
	}

	return bus->ops->init(bus);
}

stc_drv_iic_bus_t *DRV_IIC_GetBus(en_drv_iic_bus_id_t bus_id)
{
	DrvIic_ApplyDefaultConfig();
	if (Ok != DrvIic_CheckId(bus_id))
	{
		return NULL;
	}

	return &s_astDrvIicBus[bus_id];
}

void DRV_IIC_InitBus(en_drv_iic_bus_id_t bus_id)
{
	stc_drv_iic_bus_t *bus;

	bus = DRV_IIC_GetBus(bus_id);
	if (NULL != bus)
	{
		(void)DRV_IIC_Bus_Init(bus);
	}
}

void DRV_IIC_InitAll(void)
{
	uint8_t i;

	for (i = 0u; i < (uint8_t)DrvIicBusIdCount; i++)
	{
		DRV_IIC_InitBus((en_drv_iic_bus_id_t)i);
	}
}

static en_result_t DrvIic_HwInit(stc_drv_iic_bus_t *bus)
{
	en_result_t enRet;

	enRet = BSP_IIC_InitBus(bus->bsp_bus_id);
	if (Ok != enRet)
	{
		return enRet;
	}

	bus->handle = BSP_IIC_GetHandle(bus->bsp_bus_id);
	if (NULL == bus->handle)
	{
		return Error;
	}

	return Ok;
}

static void DrvIic_HwStart(stc_drv_iic_bus_t *bus)
{
	BSP_IIC_Start(bus->handle);
}

static void DrvIic_HwStop(stc_drv_iic_bus_t *bus)
{
	BSP_IIC_Stop(bus->handle);
}

static void DrvIic_HwSend(stc_drv_iic_bus_t *bus, uint8_t data)
{
	BSP_IIC_Send(bus->handle, data);
}

static void DrvIic_HwWaitAck(stc_drv_iic_bus_t *bus)
{
	BSP_IIC_WaitAck(bus->handle);
}

static uint8_t DrvIic_HwReadByte(stc_drv_iic_bus_t *bus, uint8_t ack)
{
	return BSP_IIC_ReadByte(bus->handle, ack);
}

static void DrvIic_ApplyDefaultConfig(void)
{
	if (TRUE == s_bDrvIicBusCfgDone)
	{
		return;
	}

	s_astDrvIicBus[DrvIicBusIdEeprom].bsp_bus_id = BspIicBusIdEeprom;
	s_astDrvIicBus[DrvIicBusIdEeprom].handle = BSP_IIC_GetHandle(BspIicBusIdEeprom);
	s_astDrvIicBus[DrvIicBusIdEeprom].ops = &s_stcDrvIicBusOps;
	s_astDrvIicBus[DrvIicBusIdEeprom].context = NULL;

	s_bDrvIicBusCfgDone = TRUE;
}
