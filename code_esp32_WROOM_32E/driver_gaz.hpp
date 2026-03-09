#ifndef DRIVER_GAZ_HPP
#define DRIVER_GAZ_HPP
#include "Wire.h"

#define ADDR_LUX_ETANCHE 0x

uint8_t readReg_lux_etanche(uint8_t reg, const void* pBuf, size_t size);
void setup_lux_etanche();
void readfull_lux_etanche();

void initBH1745(uint8_t address);
void readBH1745(uint8_t address, uint8_t index);
void writeReg(uint8_t address, uint8_t reg, uint8_t value);
bool readReg_BH1745(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len);

#endif