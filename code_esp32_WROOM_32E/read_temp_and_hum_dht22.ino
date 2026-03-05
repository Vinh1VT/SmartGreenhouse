#include <Arduino.h>
#include <DHT22.h>

#include "sensor_Data.h"
#include "read_temp_and_hum_dht22.h"

DHT22 pin_dht22 (PIN_DHT22);

static struct temp_and_hum_t get_temp_and_hum_dht22(void)
{
    return (struct temp_and_hum_t) {pin_dht22.getTemperature(), pin_dht22.getHumidity()};
}

void read_temp_and_hum_dht22(SensorData& data)
{
    struct temp_and_hum_t ambiant = get_temp_and_hum_dht22();

    data.temp_ambiant = (int16_t) ambiant.temp;
    data.hum_ambiant = (uint8_t) ambiant.hum;
}