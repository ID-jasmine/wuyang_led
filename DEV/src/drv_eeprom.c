#include "drv_eeprom.h"
#include "bsp_iic.h" // 调用专属于 EEPROM 的底层的 IIC 函数
#include "bsp_sys.h" // 调用 delay100us_safe

// 写入一个字节
void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
  EE_IIC_Start();
  EE_IIC_Send(EEPROM_W_ADDR);
  EE_IIC_Wait_Ack();
  EE_IIC_Send((uint8_t)addr);
  EE_IIC_Wait_Ack();
  EE_IIC_Send(data);
  EE_IIC_Wait_Ack();
  EE_IIC_Stop();
  delay100us_safe(50); // EEPROM写入需要大约5ms的内部等待时间
}

// 读取一个字节
uint8_t EEPROM_ReadByte(uint16_t addr) {
  uint8_t data = 0;
  EE_IIC_Start();
  EE_IIC_Send(EEPROM_W_ADDR);
  EE_IIC_Wait_Ack();
  EE_IIC_Send((uint8_t)addr);
  EE_IIC_Wait_Ack();

  EE_IIC_Start();
  EE_IIC_Send(EEPROM_R_ADDR);
  EE_IIC_Wait_Ack();
  data = EE_IIC_ReadByte(1); // NACK
  EE_IIC_Stop();

  return data;
}
