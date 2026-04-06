#include "driver_IA.hpp"
#include "sensor_Data.h"
#include "driver_UART.h"

extern HardwareSerial uart;

void get_data_IA(struct SensorData &data)
{
    uart.end();

    setup_UART(PATE_IA_RX, PATE_IA_TX);

    uart.println("G");

    Serial.setTimeout(10000); //timeout of 10 sec

    data.ia_1 = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);
    data.ia_2 = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);
    data.ia_3 = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);
    data.ia_anomalie = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);

    setup_UART(UART2_Rx, UART2_Tx);
}