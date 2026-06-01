#ifndef __BSP_IIC_H__
#define __BSP_IIC_H__

#include "ddl.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum en_bsp_iic_bus_id
	{
		BspIicBusIdEeprom = 0u,
		BspIicBusIdCount,
	} en_bsp_iic_bus_id_t;

	typedef struct stc_bsp_iic_handle
	{
		en_gpio_port_t scl_port;
		en_gpio_pin_t scl_pin;
		en_gpio_port_t sda_port;
		en_gpio_pin_t sda_pin;
	} IIC_Handle_t;

	en_result_t BSP_IIC_InitBus(en_bsp_iic_bus_id_t bus_id);
	IIC_Handle_t *BSP_IIC_GetHandle(en_bsp_iic_bus_id_t bus_id);
	void BSP_IIC_Start(IIC_Handle_t *iic);
	void BSP_IIC_Stop(IIC_Handle_t *iic);
	void BSP_IIC_Send(IIC_Handle_t *iic, uint8_t data);
	en_result_t BSP_IIC_WaitAck(IIC_Handle_t *iic);
	uint8_t BSP_IIC_ReadByte(IIC_Handle_t *iic, uint8_t ack);

// --- EEPROM IIC通信引脚定义 (PB6=SCL, PB7=SDA) ---
#define EE_SCL_H() Gpio_SetIO(GpioPortB, GpioPin6)
#define EE_SCL_L() Gpio_ClrIO(GpioPortB, GpioPin6)
#define EE_SDA_H() Gpio_SetIO(GpioPortB, GpioPin7)
#define EE_SDA_L() Gpio_ClrIO(GpioPortB, GpioPin7)
#define EE_SDA_IN() Gpio_GetInputIO(GpioPortB, GpioPin7)

// EEPROM IIC 基础函数
void EEPROM_IIC_Init(void);
void EE_IIC_Start(void);
void EE_IIC_Stop(void);
uint8_t EE_IIC_Wait_Ack(void);
void EE_IIC_Send(uint8_t data);
uint8_t EE_IIC_ReadByte(uint8_t ack);

#ifdef __cplusplus
}
#endif

#endif
