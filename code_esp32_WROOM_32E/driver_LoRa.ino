#include <Arduino.h>
#include "driver_UART.h"
#include "driver_LoRa.h"

static void empty_array_command_from(char *array, int len, int start)
{
    for (int i = start; i < MAX_COMMAND_LENGTH; i++)
    {
        array[i] = '\0';
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
}

static int connect_LoRa_sub(void)
{
    #if DEBUG == 1
        Serial.printf("Connecting to LoRa :\n");
    #endif
    
    write_UART("AT+JOIN");
    int status = read_until_motif_found_UART("Network joined");

    #if DEBUG == 1
        Serial.printf("\nEnd attempt.\n");
    #endif

    return (status) ? 1 : 0;
}

int connect_LoRa(void)
{
    int connection_status = 1;
    for (int i = 0; i < MAX_CONNECTION_ATEMPT && connection_status; i++)
    {
        connection_status = connect_LoRa_sub();
    }
    read_until_motif_found_UART("Done");
    
    return connection_status;
}

void write_LoRa(char *command)
{
    #if DEBUG == 1
        Serial.println(command);
    #endif
    write_UART(command);
}

char buffer_downlink[DOWNLINK_BUFFER_SIZE + 1];

int send_msg_LoRa(char *msg)
{
    char command[LoRa_MAX_MSG_LENGTH] = "AT+MSGHEX=\"";
    empty_array_command_from(command, LoRa_MAX_MSG_LENGTH, 11);

    int i = 0;
    for (i = 0; i < (LoRa_MAX_MSG_LENGTH - 13) && msg[i] != '\0'; i++)
    {
        command[i + 11] = msg[i];
    }

    command[i+11] = '"';
    command[i+12] = '\0';


    write_LoRa(command);

    int status = read_until_motif_found_UART("RX: \"");

    int indice_buff = 0;
    for (; (buffer_downlink[indice_buff] = read_byte_UART()) != '\"'; indice_buff++);
    buffer_downlink[indice_buff] = '\0';

    status = read_until_motif_found_UART("Done");

    return (status) ? 1 : 0;
}