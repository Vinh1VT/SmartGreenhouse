#include "Wire.h"
#include "driver_lux.hpp"

float LUX_VALUE[3];  // 0 = étanche, 1 = BH1745 NP1, 2 = BH1745 NP2

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
// Initialisation BH1745 (intégration 160ms + RGBC enable)
// -----------------------------------------------------------------------------
void initBH1745(uint8_t address)
{
    writeReg(address, 0x40, 0x00); // MODE_CONTROL1 : 160ms
    writeReg(address, 0x41, 0x10); // MODE_CONTROL2 : RGBC enable
    delay(200);                    // attendre fin intégration
}

// -----------------------------------------------------------------------------
// Lecture registre BH1745, retourne false si échec I2C
// -----------------------------------------------------------------------------
bool readReg_BH1745(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)  // repeated start
        return false;

    uint8_t received = Wire.requestFrom(addr, len);
    if (received != len) return false;

    for (uint8_t i = 0; i < len; i++) {
        if (Wire.available())
            data[i] = Wire.read();
        else
            return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Lecture canal CLEAR BH1745 et stockage dans LUX_VALUE[index]
// -----------------------------------------------------------------------------
void readBH1745(uint8_t address, uint8_t index)
{
    uint8_t buf[2] = {0};

    if (!readReg_BH1745(address, 0x56, buf, 2)) {
        Serial.print("Erreur I2C avec ADDR 0x");
        Serial.println(address, HEX);
        LUX_VALUE[index] = 0;
        return;
    }

    uint16_t clear = ((uint16_t)buf[1] << 8) | buf[0];
    float lux = (float)clear;

    LUX_VALUE[index] = lux;

    Serial.print("ADDR 0x");
    Serial.print(address, HEX);
    Serial.print(" -> ");
    Serial.print(lux);
    Serial.println(" lx");
}

// -----------------------------------------------------------------------------
// Lecture capteur étanche SEN0562
// -----------------------------------------------------------------------------
uint8_t readReg_lux_etanche(uint8_t reg, const void* pBuf, size_t size, int address)
{
    if (pBuf == NULL) {
        Serial.println("pBuf ERROR!! : null pointer");
        return 0;
    }
    uint8_t* _pBuf = (uint8_t*)pBuf;

    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;
    }

    delay(20);

    Wire.requestFrom(address, (uint8_t)size);
    for (uint16_t i = 0; i < size && Wire.available(); i++) {
        _pBuf[i] = Wire.read();
    }

    return size;
}

// -----------------------------------------------------------------------------
// Lecture complète de tous les capteurs et stockage dans LUX_VALUE
// -----------------------------------------------------------------------------
void readfull_lux_etanche()
{
    // Capteur étanche (SEN0562)
    uint8_t buf[2];
    readReg_lux_etanche(0x10, buf, 2, ADDR_LUX_ETANCHE);
    uint16_t data = ((uint16_t)buf[0] << 8) | buf[1];
    float Lux = ((float)data) / 1.2;
    LUX_VALUE[0] = Lux;
    Serial.print("LUX étanche: ");
    Serial.print(Lux);
    Serial.println(" lx");
    delay(1000);
    // Capteurs BH1745
    readBH1745(ADDR_LUX_NP1, 1);
    readBH1745(ADDR_LUX_NP2, 2);
}