#ifndef DRIVER_GAZ_HPP
#define DRIVER_GAZ_HPP

#include <Arduino.h>
#include <Wire.h>
#include "sensor_Data.h"
#include <DFRobot_SCD4X.h>

// -----------------------------------------------------------------------------
// Tableau global des valeurs gaz
// -----------------------------------------------------------------------------
extern float GAZ_VALUE[2];   // 0 = O2, 1 = CO2

// Adresse I2C capteur O2
#define ADDR_O2_SENSOR 0x74

// -----------------------------------------------------------------------------
// Initialisation bus I2C capteurs gaz
// -----------------------------------------------------------------------------
void setup_gaz();

// -----------------------------------------------------------------------------
// Lecture registre I2C SEN0465
// -----------------------------------------------------------------------------
bool readReg_SEN0465(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len);

// -----------------------------------------------------------------------------
// Lecture capteur O2
// -----------------------------------------------------------------------------
void read_SEN0465(uint8_t address, uint8_t index);

// -----------------------------------------------------------------------------
// Lecture complète des capteurs gaz
// -----------------------------------------------------------------------------
// Read all gas sensors and fill the provided SensorData structure
void readfull_gaz(struct SensorData &data);






/*

OBTENU DU CODE EXEMPLE DE LA DOC

*/


DFRobot_SCD4X SCD4X(&Wire, /*i2cAddr = */SCD4X_I2C_ADDR);

#endif