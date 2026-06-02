#ifndef __BSP_IIC_H__
#define __BSP_IIC_H__

#include "ddl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		BspIicBusIdEeprom = 0u,
		BspIicBusIdCount,
	} en_bsp_iic_bus_id_t;

	typedef enum
	{
		BspIicAck = 0u,
		BspIicNack = 1u,
	} en_bsp_iic_ack_t;

	typedef struct stc_bsp_iic_handle IIC_Handle_t;

	en_result_t BSP_IIC_InitBus(en_bsp_iic_bus_id_t bus_id);
	IIC_Handle_t *BSP_IIC_GetHandle(en_bsp_iic_bus_id_t bus_id);
	void BSP_IIC_Start(IIC_Handle_t *iic);
	void BSP_IIC_Stop(IIC_Handle_t *iic);
	void BSP_IIC_Send(IIC_Handle_t *iic, uint8_t data);
	en_result_t BSP_IIC_WaitAck(IIC_Handle_t *iic);
	uint8_t BSP_IIC_ReadByte(IIC_Handle_t *iic, en_bsp_iic_ack_t ack);

#ifdef __cplusplus
}
#endif

#endif
