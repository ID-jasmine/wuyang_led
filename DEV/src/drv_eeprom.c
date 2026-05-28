#include "drv_eeprom.h"
#include "bsp_iic.h"
#include "bsp_sys.h"

#define EEPROM_ADDR_WRITE 0xA0
#define EEPROM_ADDR_READ  0xA1

typedef struct {
	IIC_Handle_t *iic;
	uint8_t addr_write;
	uint8_t addr_read;
} EepromContext;

static int EEPROM_Device_Init(EepromDevice *dev);
static int EEPROM_Device_ReadBuffer(EepromDevice *dev, uint8_t wordAddress,
									uint8_t *buffer, uint16_t length);
static int EEPROM_Device_WriteBuffer(EepromDevice *dev, uint8_t wordAddress,
									 const uint8_t *buffer, uint16_t length);
static int EEPROM_HW_Init(EepromDevice *dev);
static int EEPROM_HW_ReadBuffer(EepromDevice *dev, uint8_t wordAddress,
								uint8_t *buffer, uint16_t length);
static int EEPROM_HW_WriteBuffer(EepromDevice *dev, uint8_t wordAddress,
								 const uint8_t *buffer, uint16_t length);
static int EEPROM_HW_WritePage(EepromDevice *dev, uint8_t wordAddress,
							   const uint8_t *buffer, uint8_t length);
static IIC_Handle_t *EEPROM_GetIic(EepromDevice *dev);
static EepromDevice *EEPROM_GetDefaultDevice(void);
static void EEPROM_Delay_5ms(void);

static const EepromOps s_eeprom_ops = {
	.init = EEPROM_HW_Init,
	.read_buffer = EEPROM_HW_ReadBuffer,
	.write_buffer = EEPROM_HW_WriteBuffer,
};

static EepromContext s_eeprom_context = {
	.iic = 0,
	.addr_write = EEPROM_ADDR_WRITE,
	.addr_read = EEPROM_ADDR_READ,
};

static EepromDevice s_eeprom_dev = {
	.ops = &s_eeprom_ops,
	.context = &s_eeprom_context,
	.page_size = 16,
};

int DRV_EEPROM_Init(void) {
	return EEPROM_Device_Init(EEPROM_GetDefaultDevice());
}

int DRV_EEPROM_ReadBuffer(uint8_t wordAddress, uint8_t *buffer, uint16_t length) {
	return EEPROM_Device_ReadBuffer(EEPROM_GetDefaultDevice(), wordAddress, buffer,
									length);
}

int DRV_EEPROM_WriteBuffer(uint8_t wordAddress, const uint8_t *buffer,
						   uint16_t length) {
	return EEPROM_Device_WriteBuffer(EEPROM_GetDefaultDevice(), wordAddress, buffer,
									 length);
}

static int EEPROM_Device_Init(EepromDevice *dev) {
	if (dev == 0 || dev->ops == 0 || dev->ops->init == 0) {
		return -1;
	}
	return dev->ops->init(dev);
}

static int EEPROM_Device_ReadBuffer(EepromDevice *dev, uint8_t wordAddress,
									uint8_t *buffer, uint16_t length) {
	if (dev == 0 || dev->ops == 0 || dev->ops->read_buffer == 0) {
		return -1;
	}
	return dev->ops->read_buffer(dev, wordAddress, buffer, length);
}

static int EEPROM_Device_WriteBuffer(EepromDevice *dev, uint8_t wordAddress,
									 const uint8_t *buffer, uint16_t length) {
	if (dev == 0 || dev->ops == 0 || dev->ops->write_buffer == 0) {
		return -1;
	}
	return dev->ops->write_buffer(dev, wordAddress, buffer, length);
}

static int EEPROM_HW_Init(EepromDevice *dev) {
	EepromContext *ctx;

	if (dev == 0 || dev->context == 0) {
		return -1;
	}

	ctx = (EepromContext *)dev->context;
	if (Ok != BSP_IIC_InitBus(BspIicBusIdEeprom)) {
		return -1;
	}
	ctx->iic = BSP_IIC_GetHandle(BspIicBusIdEeprom);
	if (ctx->iic == 0) {
		return -1;
	}
	return 0;
}

static int EEPROM_HW_ReadBuffer(EepromDevice *dev, uint8_t wordAddress,
								uint8_t *buffer, uint16_t length) {
	IIC_Handle_t *iic;
	EepromContext *ctx;

	if (buffer == 0) {
		return -1;
	}
	if (length == 0) {
		return 0;
	}

	iic = EEPROM_GetIic(dev);
	if (iic == 0 || dev == 0 || dev->context == 0) {
		return -1;
	}

	ctx = (EepromContext *)dev->context;
	BSP_IIC_Start(iic);
	BSP_IIC_Send(iic, ctx->addr_write);
	BSP_IIC_WaitAck(iic);
	BSP_IIC_Send(iic, wordAddress);
	BSP_IIC_WaitAck(iic);

	BSP_IIC_Start(iic);
	BSP_IIC_Send(iic, ctx->addr_read);
	BSP_IIC_WaitAck(iic);

	for (uint16_t i = 0; i < length; i++) {
		buffer[i] = BSP_IIC_ReadByte(iic, (i == length - 1) ? 1 : 0);
	}
	BSP_IIC_Stop(iic);
	return 0;
}

static int EEPROM_HW_WriteBuffer(EepromDevice *dev, uint8_t wordAddress,
								 const uint8_t *buffer, uint16_t length) {
	uint8_t page_remain;

	if (dev == 0 || buffer == 0 || dev->page_size == 0) {
		return -1;
	}
	if (length == 0) {
		return 0;
	}

	page_remain = dev->page_size - (wordAddress % dev->page_size);
	if (length <= page_remain) {
		page_remain = length;
	}

	while (1) {
		if (EEPROM_HW_WritePage(dev, wordAddress, buffer, page_remain) != 0) {
			return -1;
		}
		if (length == page_remain) {
			break;
		}

		wordAddress += page_remain;
		buffer += page_remain;
		length -= page_remain;
		page_remain = (length > dev->page_size) ? dev->page_size : length;
	}
	return 0;
}

static int EEPROM_HW_WritePage(EepromDevice *dev, uint8_t wordAddress,
							   const uint8_t *buffer, uint8_t length) {
	IIC_Handle_t *iic;
	EepromContext *ctx;

	if (dev == 0 || dev->context == 0 || buffer == 0 || length == 0 ||
		length > dev->page_size) {
		return -1;
	}

	iic = EEPROM_GetIic(dev);
	if (iic == 0) {
		return -1;
	}

	ctx = (EepromContext *)dev->context;
	BSP_IIC_Start(iic);
	BSP_IIC_Send(iic, ctx->addr_write);
	BSP_IIC_WaitAck(iic);
	BSP_IIC_Send(iic, wordAddress);
	BSP_IIC_WaitAck(iic);
	for (uint8_t i = 0; i < length; i++) {
		BSP_IIC_Send(iic, buffer[i]);
		BSP_IIC_WaitAck(iic);
	}
	BSP_IIC_Stop(iic);

	EEPROM_Delay_5ms();
	return 0;
}

static IIC_Handle_t *EEPROM_GetIic(EepromDevice *dev) {
	EepromContext *ctx;

	if (dev == 0 || dev->context == 0) {
		return 0;
	}

	ctx = (EepromContext *)dev->context;
	if (ctx->iic == 0) {
		(void)EEPROM_HW_Init(dev);
	}
	return ctx->iic;
}

static EepromDevice *EEPROM_GetDefaultDevice(void) {
	return &s_eeprom_dev;
}

static void EEPROM_Delay_5ms(void) {
	BSP_SYS_DelayMs(6u);
}
