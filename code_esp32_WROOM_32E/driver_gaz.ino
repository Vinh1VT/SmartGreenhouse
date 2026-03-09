#include "Wire.h"
#include "driver_lux.hpp"
#include "sensor_Data.h"

float GAZ_VALUE[2];  // 0 = O2, 1 = CO2

// -----------------------------------------------------------------------------
// Setup I2C pour tous les capteurs
// -----------------------------------------------------------------------------
void setup_lux_etanche()
{
    Wire.begin(21, 22);    // SDA=21, SCL=22 pour ESP32
    Wire.setClock(100000); // 100 kHz pour stabilité
}

// -----------------------------------------------------------------------------
// Ecriture registre BH1745
// -----------------------------------------------------------------------------
void writeReg(uint8_t address, uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// -----------------------------------------------------------------------------
// Initialisation SEN0465
// -----------------------------------------------------------------------------
void init_SEN0465(uint8_t address)
{
    writeReg(address, 0x41, 0x02); // MODE_CONTROL1 : 160ms
    writeReg(address, 0x42, 0x10); // MODE_CONTROL2 : RGBC enable
    writeReg(address, 0x44, 0x02);

    delay(200);                    // attendre fin intégration
}

// -----------------------------------------------------------------------------
// Lecture registre SEN0465, retourne false si échec I2C
// -----------------------------------------------------------------------------
bool readReg_SEN0465(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len)
{
    // Write register address
    Wire.beginTransmission(addr);
    Wire.write(reg);

    // Send STOP instead of repeated start
    if (Wire.endTransmission(true) != 0)
        return false;

    delayMicroseconds(50);   // small guard time (important on ESP32)

    // Request data
    uint8_t received = Wire.requestFrom((int)addr, (int)len);
    if (received != len)
        return false;

    for (uint8_t i = 0; i < len; i++) {
        data[i] = Wire.read();
    }

    return true;
}

// -----------------------------------------------------------------------------
// Lecture canal CLEAR SEN0465 et stockage dans GAZ_VALUE[index]
// -----------------------------------------------------------------------------
void read_SEN0465(uint8_t address, uint8_t index)
{
    uint8_t buf[2] = {0};

    if (!readReg_BH1745(address, 0x56, buf, 2)) {
        Serial.print("Erreur I2C avec ADDR 0x");
        Serial.println(address, HEX);
        GAZ_VALUE[index] = 0;
        return;
    }

    uint16_t clear = ((uint16_t)buf[1] << 8) | buf[0];
    float lux = (float)clear;

    GAZ_VALUE[index] = lux;

    Serial.print("ADDR 0x");
    Serial.print(address, HEX);
    Serial.print(" -> ");
    Serial.print(lux);
    Serial.println(" lx");
}

// -----------------------------------------------------------------------------
// Lecture complète de tous les capteurs et stockage dans GAZ_VALUE
// -----------------------------------------------------------------------------
void readfull_gaz()
{
    // Capteur SEN0465
    read_SEN0465(ADDR_LUX_NP1, 0); 
    // readBH1745(ADDR_LUX_NP2, 1);
}