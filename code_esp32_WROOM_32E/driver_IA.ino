#include "driver_IA.hpp"
#include "sensor_Data.h"
#include "driver_UART.h"

extern HardwareSerial uart;

void get_data_IA(struct SensorData &data)
{
    uart.end();

    uart.begin(115200, UART2_config, PATE_IA_RX, PATE_IA_TX);

    uart.println("G");

    #if DEBUG == 1
    Serial.printf("Sent G to IA card\n");
    #endif

    Serial.setTimeout(10000); //timeout of 10 sec

    data.ia_1 = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);
    data.ia_2 = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);
    data.ia_3 = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);
    data.ia_anomalie = (uint16_t) (uart.parseFloat(SKIP_ALL) * 100);

    #if DEBUG == 1
    Serial.printf("Receive %u | %u | %u | %u from IA card\n", data.ia_1, data.ia_2, data.ia_3, data.ia_anomalie);
    #endif

    uart.end();
    
    setup_UART(UART2_Rx, UART2_Tx);
}