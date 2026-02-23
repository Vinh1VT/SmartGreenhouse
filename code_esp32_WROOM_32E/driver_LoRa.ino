#include <Arduino.h>
#include "driver_UART.h"
#include "driver_LoRa.h"

static void empty_array_command(char *array, int len)
{
    for (int i = 0; i < MAX_COMMAND_LENGTH; i++)
    {
        LoRa_send_array[i] = '\0';
    }
}

void setup_LoRa(void)
{
    setup_UART();

    write_UART(AT_DR1);
    read_until_motif_found_UART("DR");

    write_UART(DEVEUI);
    read_until_motif_found_UART("ID");

    write_UART(APPEUI);
    read_until_motif_found_UART("ID");

    write_UART(APPKEY);
    read_until_motif_found_UART("KEY");

    write_UART(AT_DR2);
    read_until_motif_found_UART("DR");
    read_until_motif_found_UART("DR");

    empty_array_command();
}

int connect_LoRa(void)
{
    #if DEBUG == 1
        Serial.printf("Connecting to LoRa :\n");
    #endif
    
    write_UART("AT+JOIN");
    int status = read_until_motif_found_UART("Done");

    #if DEBUG == 1
        Serial.printf("\nEnd attempt.\n");
    #endif

    return (status) ? 1 : 0;
}

void write_LoRa(char *command)
{
    write_UART(command);
}

int send_msg_LoRa(char *msg)
{
    char command[LoRa_MAX_MSG_LENGTH];
    empty_array_command(command, LoRa_MAX_MSG_LENGTH);
    command = "AT+MSG=";

    for (int i = 0; i < (LoRa_MAX_MSG_LENGTH - 8) && msg[i] != '\0'; i++)
    {
        command[i + 7] = msg[i];
    }

    write_LoRa(command);

    int status = read_until_motif_found_UART("Done");

    return (status) ? 1 : 0;
}