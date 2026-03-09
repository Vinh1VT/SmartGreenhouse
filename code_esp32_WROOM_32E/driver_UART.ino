#include <Arduino.h>
#include <HardwareSerial.h>
#include "driver_UART.h"
#include "driver_LoRa.h"

#define MIN(i1, i2) ((i1 > i2) ? i2 : i1)

char read_buffer_uart[UART2_read_buffer_size];

HardwareSerial uart(2);

static unsigned int str_len(char *string)
{
    unsigned int len = 0;
    
    for (; string[len] != '\0'; len++);

    return len;
}

void setup_UART(void)
{
    uart.begin(UART2_baudrate, UART2_config, UART2_Rx, UART2_Tx);

    #if DEBUG == 1
        Serial.begin(Serial_baudrate);
    #endif
}

void write_UART(char *string)
{
    #if DEBUG == 1
    Serial.printf("%s\n", string);
    #endif
    uart.println(string);
}

char read_byte_UART(void)
{
    if (uart.available())
    {
        char char_read = uart.read();

        #if DEBUG == 1
            Serial.printf("%c", char_read);
        #endif
        
        return char_read;
    }else
    {
        return 0;
    }
}

int read_bytes_UART(int nb_bytes)
{
    nb_bytes = MIN(nb_bytes, UART2_read_buffer_size);
    
    for (int i = 0; i < nb_bytes && uart.available(); i++)
    {
        read_buffer_uart[i] = read_byte_UART();
    }

    return (uart.available()) ? 1 : 0;
}

int read_until_motif_found_UART(char *string)
{
    int length = str_len(string);
    unsigned int last_time = millis();
    int current = 0;
    int find_current = 0;

    while (millis() - last_time < UART2_timeout_delay && current < UART2_read_buffer_size)
    {
        char byte_read;
        if (byte_read = read_byte_UART())
        {
            read_buffer_uart[current] = byte_read;

            if (read_buffer_uart[current] == string[find_current])
            {
                find_current++;
            }else
            {
                find_current = 0;
            }

            if (find_current == length)
            {
                return 0;
            }
            
            current ++;
            last_time = millis();
        }
    }

    #if DEBUG == 1
        if (current == UART2_read_buffer_size)
        {
            Serial.printf("Read to much, buffer full\n");
        }else
        {
            Serial.printf("Timeout\n");
        }
    #endif

    return (current == UART2_read_buffer_size) ? 1 : 2;
}

void end_com(void)
{
    uart.end();
}