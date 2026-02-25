#ifndef READ_TEMP_AND_HUM_DHT22_H
#define READ_TEMP_AND_HUM_DHT22_H

#ifndef PIN_DHT22
    #define PIN_DHT22 18
#endif

struct temp_and_hum_t
{
    float temp;
    float hum;
};

struct temp_and_hum_t read_temp_and_hum_dht22(void);

#endif