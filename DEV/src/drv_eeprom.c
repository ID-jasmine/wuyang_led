#include "drv_eeprom.h"

#include "bsp_iic.h"
#include "bsp_sys.h"

#define DRV_EEPROM_ADDR_WRITE		 (0xA0u)
#define DRV_EEPROM_ADDR_READ		 (0xA1u)
#define DRV_EEPROM_WRITE_DELAY_100US (50u)

#define DRV_EEPROM_MILEAGE_PAYLOAD_SIZE	 (8u)
#define DRV_EEPROM_MILEAGE_RECORD_SIZE	 (9u)
#define DRV_EEPROM_MILEAGE_AREA_A_ADDR	 (0x00u)
#define DRV_EEPROM_MILEAGE_AREA_B_ADDR	 (0x09u)
#define DRV_EEPROM_MILEAGE_BACKUP_PERIOD (10u)

typedef struct stc_drv_eeprom_device
{
	IIC_Handle_t *iic;
	en_bsp_iic_bus_id_t bus_id;
	uint8_t addr_write;
	uint8_t addr_read;
} stc_drv_eeprom_device_t;

static stc_drv_eeprom_device_t s_stcEeprom = {
	.iic = NULL,
	.bus_id = BspIicBusIdEeprom,
	.addr_write = DRV_EEPROM_ADDR_WRITE,
	.addr_read = DRV_EEPROM_ADDR_READ,
};

static uint8_t s_u8MileageBackupCounter = 0u;

static int DRV_EEPROM_InitBus(void)
{
	if (Ok != BSP_IIC_InitBus(s_stcEeprom.bus_id))
	{
		return DRV_EEPROM_ERROR;
	}

	s_stcEeprom.iic = BSP_IIC_GetHandle(s_stcEeprom.bus_id);
	if (NULL == s_stcEeprom.iic)
	{
		return DRV_EEPROM_ERROR;
	}

	return 0;
}

static int DRV_EEPROM_EnsureReady(void)
{
	if (NULL != s_stcEeprom.iic)
	{
		return 0;
	}

	return DRV_EEPROM_InitBus();
}

static uint8_t DRV_EEPROM_CalcChecksum(const uint8_t *data, uint8_t len)
{
	uint8_t checksum = 0u;
	uint8_t i;

	for (i = 0u; i < len; i++)
	{
		checksum ^= data[i];
	}

	return checksum;
}

static void DRV_EEPROM_PackMileage(const stc_drv_eeprom_mileage_t *mileage, uint8_t *buf)
{
	buf[0] = (uint8_t)(mileage->total_tenths_km >> 24);
	buf[1] = (uint8_t)(mileage->total_tenths_km >> 16);
	buf[2] = (uint8_t)(mileage->total_tenths_km >> 8);
	buf[3] = (uint8_t)mileage->total_tenths_km;
	buf[4] = (uint8_t)(mileage->trip_tenths_km >> 8);
	buf[5] = (uint8_t)mileage->trip_tenths_km;
	buf[6] = mileage->unit_imperial;
	buf[7] = mileage->display_trip;
}

static void DRV_EEPROM_UnpackMileage(const uint8_t *buf, stc_drv_eeprom_mileage_t *mileage)
{
	mileage->total_tenths_km = ((uint32_t)buf[0] << 24);
	mileage->total_tenths_km |= ((uint32_t)buf[1] << 16);
	mileage->total_tenths_km |= ((uint32_t)buf[2] << 8);
	mileage->total_tenths_km |= buf[3];
	mileage->trip_tenths_km = (uint16_t)(((uint16_t)buf[4] << 8) | buf[5]);
	mileage->unit_imperial = buf[6];
	mileage->display_trip = buf[7];
}

static int DRV_EEPROM_ReadRecord(uint8_t start_addr, uint8_t *buf)
{
	uint8_t i;

	for (i = 0u; i < DRV_EEPROM_MILEAGE_RECORD_SIZE; i++)
	{
		if (0 != DRV_EEPROM_ReadByte((uint16_t)(start_addr + i), &buf[i]))
		{
			return DRV_EEPROM_ERROR;
		}
	}

	return 0;
}

static int DRV_EEPROM_WriteRecord(uint8_t start_addr,
								  const stc_drv_eeprom_mileage_t *mileage)
{
	uint8_t buf[DRV_EEPROM_MILEAGE_RECORD_SIZE];
	uint8_t i;

	DRV_EEPROM_PackMileage(mileage, buf);
	buf[DRV_EEPROM_MILEAGE_PAYLOAD_SIZE] =
		DRV_EEPROM_CalcChecksum(buf, DRV_EEPROM_MILEAGE_PAYLOAD_SIZE);

	for (i = 0u; i < DRV_EEPROM_MILEAGE_RECORD_SIZE; i++)
	{
		if (0 != DRV_EEPROM_WriteByte((uint16_t)(start_addr + i), buf[i]))
		{
			return DRV_EEPROM_ERROR;
		}
	}

	return 0;
}

static int DRV_EEPROM_IsRecordValid(const uint8_t *buf)
{
	if (buf[DRV_EEPROM_MILEAGE_PAYLOAD_SIZE] !=
		DRV_EEPROM_CalcChecksum(buf, DRV_EEPROM_MILEAGE_PAYLOAD_SIZE))
	{
		return 0;
	}

	return 1;
}

static int DRV_EEPROM_IsLegacyPayloadPresent(const uint8_t *buf)
{
	uint8_t i;

	for (i = 0u; i < DRV_EEPROM_MILEAGE_PAYLOAD_SIZE; i++)
	{
		if (0xFFu != buf[i])
		{
			return 1;
		}
	}

	return 0;
}

int DRV_EEPROM_Init(void)
{
	return DRV_EEPROM_InitBus();
}

int DRV_EEPROM_WriteByte(uint16_t addr, uint8_t data)
{
	int ret = DRV_EEPROM_ERROR;

	if (0 != DRV_EEPROM_EnsureReady())
	{
		return DRV_EEPROM_ERROR;
	}

	BSP_IIC_Start(s_stcEeprom.iic);
	BSP_IIC_Send(s_stcEeprom.iic, s_stcEeprom.addr_write);
	if (Ok != BSP_IIC_WaitAck(s_stcEeprom.iic))
	{
		goto out;
	}

	BSP_IIC_Send(s_stcEeprom.iic, (uint8_t)addr);
	if (Ok != BSP_IIC_WaitAck(s_stcEeprom.iic))
	{
		goto out;
	}

	BSP_IIC_Send(s_stcEeprom.iic, data);
	if (Ok != BSP_IIC_WaitAck(s_stcEeprom.iic))
	{
		goto out;
	}

	ret = 0;

out:
	BSP_IIC_Stop(s_stcEeprom.iic);
	if (0 == ret)
	{
		delay100us_safe(DRV_EEPROM_WRITE_DELAY_100US);
	}
	return ret;
}

int DRV_EEPROM_ReadByte(uint16_t addr, uint8_t *data)
{
	int ret = DRV_EEPROM_ERROR;

	if (NULL == data)
	{
		return DRV_EEPROM_ERROR;
	}
	if (0 != DRV_EEPROM_EnsureReady())
	{
		return DRV_EEPROM_ERROR;
	}

	BSP_IIC_Start(s_stcEeprom.iic);
	BSP_IIC_Send(s_stcEeprom.iic, s_stcEeprom.addr_write);
	if (Ok != BSP_IIC_WaitAck(s_stcEeprom.iic))
	{
		goto out;
	}

	BSP_IIC_Send(s_stcEeprom.iic, (uint8_t)addr);
	if (Ok != BSP_IIC_WaitAck(s_stcEeprom.iic))
	{
		goto out;
	}

	BSP_IIC_Start(s_stcEeprom.iic);
	BSP_IIC_Send(s_stcEeprom.iic, s_stcEeprom.addr_read);
	if (Ok != BSP_IIC_WaitAck(s_stcEeprom.iic))
	{
		goto out;
	}

	*data = BSP_IIC_ReadByte(s_stcEeprom.iic, BspIicNack);
	ret = 0;

out:
	BSP_IIC_Stop(s_stcEeprom.iic);
	return ret;
}

int DRV_EEPROM_LoadMileage(stc_drv_eeprom_mileage_t *mileage)
{
	uint8_t buf[DRV_EEPROM_MILEAGE_RECORD_SIZE];

	if (NULL == mileage)
	{
		return DRV_EEPROM_ERROR;
	}

	if (0 != DRV_EEPROM_ReadRecord(DRV_EEPROM_MILEAGE_AREA_A_ADDR, buf))
	{
		return DRV_EEPROM_ERROR;
	}
	if (0 != DRV_EEPROM_IsRecordValid(buf))
	{
		DRV_EEPROM_UnpackMileage(buf, mileage);
		return DRV_EEPROM_LOAD_VALID;
	}

	if (0 != DRV_EEPROM_ReadRecord(DRV_EEPROM_MILEAGE_AREA_B_ADDR, buf))
	{
		return DRV_EEPROM_ERROR;
	}
	if (0 != DRV_EEPROM_IsRecordValid(buf))
	{
		DRV_EEPROM_UnpackMileage(buf, mileage);
		(void)DRV_EEPROM_WriteRecord(DRV_EEPROM_MILEAGE_AREA_A_ADDR, mileage);
		return DRV_EEPROM_LOAD_VALID;
	}

	if (0 != DRV_EEPROM_ReadRecord(DRV_EEPROM_MILEAGE_AREA_A_ADDR, buf))
	{
		return DRV_EEPROM_ERROR;
	}
	if (0 != DRV_EEPROM_IsLegacyPayloadPresent(buf))
	{
		DRV_EEPROM_UnpackMileage(buf, mileage);
		return DRV_EEPROM_LOAD_LEGACY;
	}

	return DRV_EEPROM_LOAD_EMPTY;
}

int DRV_EEPROM_SaveMileage(const stc_drv_eeprom_mileage_t *mileage)
{
	if (NULL == mileage)
	{
		return DRV_EEPROM_ERROR;
	}

	if (0 != DRV_EEPROM_WriteRecord(DRV_EEPROM_MILEAGE_AREA_A_ADDR, mileage))
	{
		return DRV_EEPROM_ERROR;
	}

	s_u8MileageBackupCounter++;
	if (s_u8MileageBackupCounter >= DRV_EEPROM_MILEAGE_BACKUP_PERIOD)
	{
		s_u8MileageBackupCounter = 0u;
		if (0 != DRV_EEPROM_WriteRecord(DRV_EEPROM_MILEAGE_AREA_B_ADDR, mileage))
		{
			return DRV_EEPROM_ERROR;
		}
	}

	return 0;
}
