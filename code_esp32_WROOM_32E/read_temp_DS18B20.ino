#include <Arduino.h>
#include <OneWire.h>

#include "read_temp_DS18B20.h"

OneWire ds(PIN_ONE_WIRE_DS18B20);

extern byte addr[NB_SENSORS_DS18B20][8];
double data_ds18b20[NB_SENSORS_DS18B20] = {0};

static double convert_binary_to_double(byte ms, byte ls)
{
    int data = ((int) ms << 8) + ls;

    double sum = 0;

    for (int i = 0; i < 10; i++)
    {
        sum += ((data & (1 << i)) ? 1 : 0) * ((i > 4) ? (1 << (i - 4)) : 1.0/((double) (1 << (4 - i))));
    }

    return sum;
}

#if DEBUG == 1
void print_addr(void)
{
    Serial.printf("Start :\n");

    byte address[8];

    Serial.printf("Begin read address\n");

    while (ds.search(address))
    {
        Serial.printf("{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}\n", address[0], address[1], address[2], address[3], address[4], address[5], address[6], address[7]);
    }

    Serial.printf("End read address\n");
}
#endif

void read_temperature_ds18b20(void)
{
    byte data_read[9] = {0};
    for (int i = 0; i < NB_SENSORS_DS18B20; i++)
    {
        ds.reset();
        ds.select(addr[i]);
        ds.write(0x44);

        delay(250);

        ds.reset();
        ds.select(addr[i]);
        ds.write(0xBE);

        for (int j = 0; j < 9; j++) {
          data_read[j] = ds.read();
        }
    
        data_ds18b20[i] = convert_binary_to_double(data_read[1], data_read[0]);
    }
}