#ifndef READ_TEMP_AND_HUM_DHT22_H
#define READ_TEMP_AND_HUM_DHT22_H

#include "sensor_Data.h"

#ifndef PIN_DHT22
    #define PIN_DHT22 18
#endif

struct temp_and_hum_t
{
    float temp;
    float hum;
};

void read_temp_and_hum_dht22(SensorData& data);

#endif