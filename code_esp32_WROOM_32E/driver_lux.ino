#include "Wire.h"
#include "driver_lux.hpp"

void setup_lux_etanche()
{
  Serial.begin(9600);
  Wire.begin();
}
uint8_t buf[4] = {0};
uint16_t data, data1;
float Lux;
void readfull_lux_etanche()
{
  readReg(0x10, buf, 2);              //Register address 0x10
  data = buf[0] << 8 | buf[1];
  Lux = (((float)data )/1.2);
  Serial.print("LUX:");
  Serial.print(Lux);
  Serial.print("lx");
  Serial.print("\n");
}
uint8_t readReg_lux_etanche(uint8_t reg, const void* pBuf, size_t size)
{
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t * _pBuf = (uint8_t *)pBuf;
  Wire.beginTransmission(ADDR_LUX_ETANCHE);
  Wire.write(®, 1);
  if ( Wire.endTransmission() != 0) {
    return 0;
  }
  delay(20);
  Wire.requestFrom(ADDR_LUX_ETANCHE, (uint8_t) size);
  for (uint16_t i = 0; i < size; i++) {
    _pBuf[i] = Wire.read();
  }
  return size;
}
