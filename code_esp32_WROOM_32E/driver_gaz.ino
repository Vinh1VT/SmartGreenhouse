#include "driver_gaz.hpp"

float GAZ_VALUE[2];  // 0 = O2, 1 = CO2

bool readReg_SEN0465(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    // send register address; use a stop condition to be compatible with devices
    if (Wire.endTransmission() != 0)
        return false;

    // short pause to allow the sensor to prepare data
    delay(5);

    uint8_t received = Wire.requestFrom((int)addr, (int)len);
    if (received != len)
        return false;

    for (uint8_t i = 0; i < len; i++)
        data[i] = Wire.read();

    return true;
}

void read_SEN0465(uint8_t address, uint8_t index)
{
    uint8_t buf[2];
    const uint8_t tries = 3;
    bool ok = false;
    uint16_t raw = 0;

    for (uint8_t t = 0; t < tries; ++t) {
        Serial.printf("SEN0465: try %d read reg 0x03\n", t);
        if (readReg_SEN0465(address, 0x03, buf, 2)) {
            raw = ((uint16_t)buf[0] << 8) | buf[1];
            Serial.printf("  raw bytes: %02X %02X -> raw=%u\n", buf[0], buf[1], raw);
            // accept non-zero raw as valid
            if (raw != 0) { ok = true; break; }
        } else {
            Serial.println("  readReg_SEN0465 failed on 0x03");
        }
        delay(10);
    }



    if (!ok) {
        Serial.println("Erreur lecture capteur O2 (no valid data)");
        GAZ_VALUE[index] = 0.0f;
        return;
    }

    GAZ_VALUE[index] = raw / 10.0f; // sensor gives value in tenths
    Serial.printf("SEN0465: computed O2=%0.1f (from raw=%u)\n", GAZ_VALUE[index], raw);
}




void setup_gaz()
{
    // Ensure I2C is initialized (SDA=21, SCL=22) and set clock
    Wire.begin(21, 22);
    Wire.setClock(100000);

    // Check presence of O2 sensor
    Wire.beginTransmission(ADDR_O2_SENSOR);
    if (Wire.endTransmission() == 0) {
        Serial.println("O2 sensor detected at 0x74");
    } else {
        Serial.println("O2 sensor not detected at 0x74");
    }

    // Run diagnostics to dump several registers and a raw read
    Serial.println("SEN0465: running diagnostics");
    // Dump registers 0x00..0x0F (two bytes each)
    for (uint8_t reg = 0x00; reg <= 0x0F; ++reg) {
        uint8_t buf[2] = {0xFF, 0xFF};
        bool ok = readReg_SEN0465(ADDR_O2_SENSOR, reg, buf, 2);
        Serial.printf(" reg 0x%02X: ", reg);
        if (ok) {
            Serial.printf("%02X %02X\n", buf[0], buf[1]);
        } else {
            Serial.println("-- read failed --");
        }
        delay(5);
    }

    // Try requesting data without writing a register (some devices do this)
    Serial.println("SEN0465: try raw requestFrom without register write");
    Wire.requestFrom((int)ADDR_O2_SENSOR, (int)2);
    if (Wire.available()) {
        Serial.print(" raw: ");
        while (Wire.available()) {
            Serial.printf("%02X ", Wire.read());
        }
        Serial.println();
    } else {
        Serial.println(" raw: no bytes available");
    }
}

void readfull_gaz(struct SensorData &data)
{
    // Read O2 sensor and store in provided data struct
    read_SEN0465(ADDR_O2_SENSOR, 0);
    data.o2 = (uint16_t)GAZ_VALUE[0];
}