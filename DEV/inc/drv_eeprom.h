#ifndef __DRV_EEPROM_H
#define __DRV_EEPROM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DRV_EEPROM_LOAD_VALID  (0)
#define DRV_EEPROM_LOAD_LEGACY (1)
#define DRV_EEPROM_LOAD_EMPTY  (-1)
#define DRV_EEPROM_ERROR	   (-2)

typedef struct stc_drv_eeprom_mileage
{
	uint32_t total_tenths_km;
	uint16_t trip_tenths_km;
	uint8_t unit_imperial;
	uint8_t display_trip;
} stc_drv_eeprom_mileage_t;

int DRV_EEPROM_Init(void);
int DRV_EEPROM_ReadByte(uint16_t addr, uint8_t *data);
int DRV_EEPROM_WriteByte(uint16_t addr, uint8_t data);
int DRV_EEPROM_LoadMileage(stc_drv_eeprom_mileage_t *mileage);
int DRV_EEPROM_SaveMileage(const stc_drv_eeprom_mileage_t *mileage);

#ifdef __cplusplus
}
#endif

#endif
