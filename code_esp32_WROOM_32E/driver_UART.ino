#include <Arduino.h>
#include <HardwareSerial.h>
#include "driver_uart.h"

HardwareSerial uart(2);

void setup_UART(void)
{
    uart.begin(UART2_baudrate, UART2_config, UART2_Rx, UART2_Tx);

    #if DEBUG == 1
        Serial.begin(Serial_baudrate);
    #endif
}

void write_UART(char *string)
{
    uart.println(string);
}