#include <Arduino.h>

#define DEBUG 1

#include "driver_UART.h"
#include "driver_LoRa.h"
#include "driver_ana.hpp"
#include "read_temp_DS18B20.h"
#include "read_temp_and_hum_dht22.h"
#include "sensor_Data.h"

void setup(void)
{
    setup_LoRa();
    initADC36();
}

extern double data_ds18b20[NB_SENSORS_DS18B20];
extern float ANA1_CAPA[8];

void loop(void)
#if DEBUG != 1
{
    
}
#else
{
    delay(1000);
    Serial.printf("Start :\n");

    Serial.printf("Start reading DS18B20 :\n");
    Serial.printf("Read address :\n");
    print_addr();
    Serial.printf("Read temp :\n");
    read_temperature_ds18b20();
    for (int i = 0; i < NB_SENSORS_DS18B20; i++)
    {
        Serial.printf("%f ", data_ds18b20[i]);
    }
    Serial.printf("\n\n");

    Serial.printf("Start reading DHT22 :\n");
    struct temp_and_hum_t read_dht22 = read_temp_and_hum_dht22();
    Serial.printf("temp : %f | hum : %f\n\n", read_dht22.temp, read_dht22.hum);

    Serial.printf("Start reading capacity voltage (for now):\n");
    readADC36();
    Serial.printf("Test reading from the shared array :\n");
    for (int i = 0; i < 8; i++)
    {
        Serial.printf("%f ", ANA1_CAPA[i]);
    }
    Serial.printf("\n\n");

    esp_sleep_enable_timer_wakeup(10 * 1000000);
    esp_deep_sleep_start();

    //while(1);
}
#endif