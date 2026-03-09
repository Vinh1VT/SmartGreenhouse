#include <Arduino.h>
#include <cstdio>

#define DEBUG 1

#include "driver_UART.h"
#include "driver_LoRa.h"
#include "driver_ana.hpp"
#include "read_temp_DS18B20.h"
#include "read_temp_and_hum_dht22.h"
#include "sensor_Data.h"

#define MULT_S_TO_MIN 60

#define PAYLOAD_BUFF_LEN 256
#define PAYLOAD_HEXBUFF_LEN PAYLOAD_BUFF_LEN * 2

void setup(void)
{
    void switch_load_init();
    delay(100); /* laisse le temps aux périphériques de s'alimenter (jsp si c'est necessaire)*/
    setup_LoRa();
    initADC36();
}

void loop(void)
{
    #if DEBUG == 1
        Serial.printf("\n---------------\n- LOOP START : -\n---------------\n");
    #endif
    
    /* Connecte au reseau LoRa */
    if (connect_LoRa())
    {
        /* connection échoué à voir ce que l'on fait */
        start_deep_sleep(30);
    }

    #if DEBUG == 1
        print_addr(); /* recupere les adresse one wire des capteurs DS18B20 */
    #endif
    
    /* Recupere les données */
    struct SensorData data;

    readADC36(data);
    read_temperature_ds18b20(data);

    /* prepare the payload */
    uint8_t payload_buff[PAYLOAD_BUFF_LEN];
    int length = buildPayload(data, payload_buff);

    #if DEBUG == 1
        printf("payload :\n[");
        for (int i = 0; i < length; i++)
        {
            printf("%d, ", payload_buff[i]);
        }
        printf("]\n");
    #endif
    
    char payload_hexbuff[PAYLOAD_HEXBUFF_LEN];
    arrayToHex(payload_buff, length, payload_hexbuff);
    
    /* send the message */
    send_msg_LoRa(payload_hexbuff);

    #if DEBUG == 1
        Serial.printf("\n------------\n- LOOP STOP -\n------------\n");
    #endif

    /* Lance le deep sleep */
    start_deep_sleep(30);
}