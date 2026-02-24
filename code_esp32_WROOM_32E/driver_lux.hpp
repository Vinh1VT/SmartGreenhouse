#ifndef DRIVER_LUX_HPP
#define DRIVER_LUX_HPP
#include "Wire.h"

#define ADDR_LUX_ETANCHE 0x23                 //I2C address 0x23

uint8_t readReg_lux_etanche(uint8_t reg, const void* pBuf, size_t size);
void setup_lux_etanche();
void readfull_lux_etanche();

#endif