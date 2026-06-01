#ifndef __DRV_EEPROM_H
#define __DRV_EEPROM_H

#include <stdint.h>

// EEPROM 读写地址
#define EEPROM_W_ADDR 0xA0
#define EEPROM_R_ADDR 0xA1

void EEPROM_WriteByte(uint16_t addr, uint8_t data);
uint8_t EEPROM_ReadByte(uint16_t addr);

#endif
