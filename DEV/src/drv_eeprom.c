#include "drv_eeprom.h"
#include "bsp_iic.h"
#include "bsp_sys.h"

static int EEPROM_EnsureReady(EepromDevice *dev)
{
	if (dev == 0 || dev->ops == 0 || dev->ops->init_bus == 0 || dev->ops->get_handle == 0)
	{
		return -1;
	}

	if (dev->iic != 0)
	{
		return 0;
	}

	if (Ok != dev->ops->init_bus(dev->bus_id))
	{
		return -1;
	}

	dev->iic = dev->ops->get_handle(dev->bus_id);
	if (dev->iic == 0)
	{
		return -1;
	}

	return 0;
}

static int EEPROM_ReadBuffer(EepromDevice *dev, uint8_t wordAddress, uint8_t *buffer,
							 uint16_t length)
{
	if (buffer == 0)
	{
		return -1;
	}
	if (length == 0)
	{
		return 0;
	}
	if (EEPROM_EnsureReady(dev) != 0)
	{
		return -1;
	}

	dev->ops->start(dev->iic);
	dev->ops->send(dev->iic, dev->addr_write);
	if (dev->ops->wait_ack(dev->iic) != Ok)
	{
		return -1;
	}
	dev->ops->send(dev->iic, wordAddress);
	if (dev->ops->wait_ack(dev->iic) != Ok)
	{
		return -1;
	}

	dev->ops->start(dev->iic);
	dev->ops->send(dev->iic, dev->addr_read);
	if (dev->ops->wait_ack(dev->iic) != Ok)
	{
		return -1;
	}

	for (uint16_t i = 0u; i < length; i++)
	{
		buffer[i] = dev->ops->read_byte(dev->iic, (i == (length - 1u)) ? 1u : 0u);
	}

	dev->ops->stop(dev->iic);
	return 0;
}

static int EEPROM_WriteBuffer(EepromDevice *dev, uint8_t wordAddress,
							  const uint8_t *buffer, uint16_t length)
{
	uint8_t write_len;

	if (dev == 0 || buffer == 0 || dev->page_size == 0u)
	{
		return -1;
	}
	if (length == 0u)
	{
		return 0;
	}
	if (EEPROM_EnsureReady(dev) != 0)
	{
		return -1;
	}

	write_len = dev->page_size - (wordAddress % dev->page_size);
	if (length < write_len)
	{
		write_len = (uint8_t)length;
	}

	while (length > 0u)
	{
		if (EEPROM_WritePage(dev, wordAddress, buffer, write_len) != 0)
		{
			return -1;
		}

		wordAddress += write_len;
		buffer += write_len;
		length -= write_len;
		write_len = (length > dev->page_size) ? dev->page_size : (uint8_t)length;
	}

	return 0;
}

static int EEPROM_WritePage(EepromDevice *dev, uint8_t wordAddress, const uint8_t *buffer,
							uint8_t length)
{
	if (dev == 0 || buffer == 0 || length == 0u || length > dev->page_size ||
		EEPROM_EnsureReady(dev) != 0)
	{
		return -1;
	}

	dev->ops->start(dev->iic);
	dev->ops->send(dev->iic, dev->addr_write);
	if (dev->ops->wait_ack(dev->iic) != Ok)
	{
		return -1;
	}
	dev->ops->send(dev->iic, wordAddress);
	if (dev->ops->wait_ack(dev->iic) != Ok)
	{
		return -1;
	}

	for (uint8_t i = 0u; i < length; i++)
	{
		dev->ops->send(dev->iic, buffer[i]);
		if (dev->ops->wait_ack(dev->iic) != Ok)
		{
			return -1;
		}
	}

	dev->ops->stop(dev->iic);
	dev->ops->delay_ms(6u);
	return 0;
}

static const EepromIicOps s_eeprom_iic_ops = {
	.init_bus = BSP_IIC_InitBus,
	.get_handle = BSP_IIC_GetHandle,
	.start = BSP_IIC_Start,
	.stop = BSP_IIC_Stop,
	.send = BSP_IIC_Send,
	.wait_ack = BSP_IIC_WaitAck,
	.read_byte = BSP_IIC_ReadByte,
	.delay_ms = BSP_SYS_DelayMs,
};

static EepromDevice s_eeprom = {
	.ops = &s_eeprom_iic_ops,
	.iic = 0,
	.bus_id = BspIicBusIdEeprom,
	.addr_write = EEPROM_ADDR_WRITE,
	.addr_read = EEPROM_ADDR_READ,
	.page_size = EEPROM_PAGE_SIZE,
};

int DRV_EEPROM_Init(void)
{
	return EEPROM_EnsureReady(&s_eeprom);
}

int DRV_EEPROM_ReadBuffer(uint8_t wordAddress, uint8_t *buffer, uint16_t length)
{
	return EEPROM_ReadBuffer(&s_eeprom, wordAddress, buffer, length);
}

int DRV_EEPROM_WriteBuffer(uint8_t wordAddress, const uint8_t *buffer, uint16_t length)
{
	return EEPROM_WriteBuffer(&s_eeprom, wordAddress, buffer, length);
}
