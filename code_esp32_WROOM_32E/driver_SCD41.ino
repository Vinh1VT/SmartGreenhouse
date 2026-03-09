#include "driver_SCD41.hpp"
#include "sensor_Data.h"

// Instantiate SCD41
DFRobot_SCD4X SCD41(&Wire, SCD4X_I2C_ADDR);

/**
 * @brief Setup SCD41 sensor (call once in setup)
 */
void setup_scd41()
{
    Wire.begin(21, 22);       // Use same I2C pins as other sensors
    Wire.setClock(100000);    // 100 kHz standard I2C

    // Initialize sensor
    while(!SCD41.begin()) {
        Serial.println("SCD41 init failed, check wiring");
        delay(2000);
    }
    Serial.println("SCD41 ready");

    // Stop any previous measurement
    SCD41.enablePeriodMeasure(SCD4X_STOP_PERIODIC_MEASURE);

    // Optional: adjust temperature compensation and altitude
    SCD41.setTempComp(0.0);
    SCD41.setSensorAltitude(0);

    // Start low-power periodic measurement (~30s update interval)
    SCD41.enablePeriodMeasure(SCD4X_START_LOW_POWER_MEASURE);
}

/**
 * @brief Read SCD41 measurements (call in loop)
 */
void read_scd41(struct SensorData &data)
{
    while(!SCD41.getDataReadyStatus()) {
        
    }
    DFRobot_SCD4X::sSensorMeasurement_t meas;
        SCD41.readMeasurement(&meas);

        data.hum_ambiant = meas.humidity;
        data.co2 = meas.CO2ppm;
        data.temp_ambiant = meas.temp;

        Serial.printf("SCD41: CO2=%u ppm, T=%.2f C, RH=%.2f %%\n",
                      meas.CO2ppm, meas.temp, meas.humidity);
}