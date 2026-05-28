#ifndef __DRV_IIC_H__
#define __DRV_IIC_H__

#include "bsp_iic.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct stc_drv_iic_bus stc_drv_iic_bus_t;

typedef struct stc_drv_iic_bus_ops
{
	en_result_t (*init)(stc_drv_iic_bus_t *bus);
	void (*start)(stc_drv_iic_bus_t *bus);
	void (*stop)(stc_drv_iic_bus_t *bus);
	void (*send)(stc_drv_iic_bus_t *bus, uint8_t data);
	void (*wait_ack)(stc_drv_iic_bus_t *bus);
	uint8_t (*read_byte)(stc_drv_iic_bus_t *bus, uint8_t ack);
} stc_drv_iic_bus_ops_t;

struct stc_drv_iic_bus
{
	en_bsp_iic_bus_id_t bsp_bus_id;
	IIC_Handle_t *handle;
	const stc_drv_iic_bus_ops_t *ops;
	void *context;
};

typedef enum en_drv_iic_bus_id
{
	DrvIicBusIdEeprom = 0u,
	DrvIicBusIdCount,
} en_drv_iic_bus_id_t;

typedef stc_drv_iic_bus_t DRV_IIC_Bus;
typedef en_drv_iic_bus_id_t DRV_IIC_BusId_t;

en_result_t DRV_IIC_Bus_Init(stc_drv_iic_bus_t *bus);
stc_drv_iic_bus_t *DRV_IIC_GetBus(en_drv_iic_bus_id_t bus_id);
void DRV_IIC_InitBus(en_drv_iic_bus_id_t bus_id);
void DRV_IIC_InitAll(void);

#define DRV_IIC_BUS_EEPROM DrvIicBusIdEeprom
#define DRV_IIC_BUS_COUNT DrvIicBusIdCount

#ifdef __cplusplus
}
#endif

#endif
