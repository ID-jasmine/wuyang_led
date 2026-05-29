#ifndef __DRV_EEPROM_H
#define __DRV_EEPROM_H

#include "bsp_iic.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define EEPROM_ADDR_WRITE 0xA0u
#define EEPROM_ADDR_READ  0xA1u
#define EEPROM_PAGE_SIZE  16u

	typedef struct
	{
		en_result_t (*init_bus)(en_bsp_iic_bus_id_t bus_id);
		IIC_Handle_t *(*get_handle)(en_bsp_iic_bus_id_t bus_id);
		void (*start)(IIC_Handle_t *iic);
		void (*stop)(IIC_Handle_t *iic);
		void (*send)(IIC_Handle_t *iic, uint8_t data);
		en_result_t (*wait_ack)(IIC_Handle_t *iic);
		uint8_t (*read_byte)(IIC_Handle_t *iic, uint8_t ack);
		void (*delay_ms)(uint32_t ms);
	} EepromIicOps;

	typedef struct
	{
		const EepromIicOps *ops;
		IIC_Handle_t *iic;
		en_bsp_iic_bus_id_t bus_id;
		uint8_t addr_write;
		uint8_t addr_read;
		uint8_t page_size;
	} EepromDevice;

	int DRV_EEPROM_Init(void);
	int DRV_EEPROM_ReadBuffer(uint8_t wordAddress, uint8_t *buffer, uint16_t length);
	int DRV_EEPROM_WriteBuffer(uint8_t wordAddress, const uint8_t *buffer,
							   uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
