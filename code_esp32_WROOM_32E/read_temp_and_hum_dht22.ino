#include <Arduino.h>
#include <DHT22.h>

#include "read_temp_and_hum_dht22.h"

DHT22 pin_dht22 (PIN_DHT22);

struct temp_and_hum_t read_temp_and_hum(void)
{
    return (struct temp_and_hum_t) {pin_dht22.getTemperature(), pin_dht22.getHumidity()};
}