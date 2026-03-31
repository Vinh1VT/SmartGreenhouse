#include <Arduino.h>
#include <cstdio>
#include <stdio.h>

#define DEBUG 1

#include "driver_UART.h"
#include "driver_LoRa.h"
#include "driver_ana.hpp"
#include "read_temp_DS18B20.h"
#include "sensor_Data.h"
#include "deep_sleep.hpp"
#include "driver_SCD41.hpp"
#include "driver_lux.hpp"

#define MULT_S_TO_MIN 60
#define SLEEP_TIME 30 * MULT_S_TO_MIN

#define PAYLOAD_BUFF_LEN 256
#define PAYLOAD_HEXBUFF_LEN PAYLOAD_BUFF_LEN * 2

int print_wakeup_reason(void) {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:    return 1; break;
    default:                        return 0; break;
  }
}


int convert_from_hex(char c)
{
    switch(c)
    {
        case '0' :
            return 0;
        case '1' :
            return 1;
        case '2' :
            return 2;
        case '3' :
            return 3;
        case '4' :
            return 4;
        case '5' :
            return 5;
        case '6' :
            return 6;
        case '7' :
            return 7;
        case '8' :
            return 8;
        case '9' :
            return 9;
        case 'A' :
            return 10;
        case 'B' :
            return 11;
        case 'C' :
            return 12;
        case 'D' :
            return 13;
        case 'E' :
            return 14;
        case 'F' :
            return 15;
        default :
            return 0;
    }
}

int scd41_status = 0;

struct SensorData data;

void setup(void)
{
    switch_load_init();
    delay(500); /* laisse le temps aux périphériques de s'alimenter (jsp si c'est necessaire)*/
    setup_LoRa();
    initADC36();
    scd41_status = setup_scd41();
    
    // initBH1745(ADDR_LUX_NP1);

    int wakeup_reason = print_wakeup_reason();
    if (!wakeup_reason)
    {
        pinMode(4, OUTPUT);
        digitalWrite(4, 1);
        delay(1000);
        digitalWrite(4, 0);
    }
}

extern char buffer_downlink[DOWNLINK_BUFFER_SIZE + 1];

void loop(void)
{
    #if DEBUG == 1
        Serial.printf("\n----------------\n- LOOP START : -\n----------------\n");
    #endif

    // print_wakeup_reason();
    
    /* Connecte au reseau LoRa */
    if (connect_LoRa())
    {
        /* connection échoué à voir ce que l'on fait */
        start_deep_sleep(SLEEP_TIME);
    }

    #if DEBUG == 1
        print_addr(); /* recupere les adresse one wire des capteurs DS18B20 */
    #endif
    
    /* Recupere les données */
    

    readADC36(data);
    read_temperature_ds18b20(data);
    if (scd41_status)
    {
        read_scd41(data);
    }
    
    readfull_lux_etanche(data);
    
    /* prepare the payload */
    uint8_t payload_buff[PAYLOAD_BUFF_LEN];
    int length = buildPayload(data, payload_buff);

    #if DEBUG == 1
        printf("\npayload :\n[");
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

    if (buffer_downlink[0] != '\0' && buffer_downlink[1] != '\0')
    {
        setFanSpeedPercent(convert_from_hex(buffer_downlink[0]) * 16 + convert_from_hex(buffer_downlink[1]));

        #if DEBUG == 1
        Serial.printf("translated : %d\n", convert_from_hex(buffer_downlink[0]) * 16 + convert_from_hex(buffer_downlink[1]));
        #endif
    }
    
    // end_com();

    #if DEBUG == 1
        Serial.printf("\n-------------\n- LOOP STOP -\n-------------\n");
        Serial.flush();
    #endif

    // while(1);

    /* Lance le deep sleep */
    start_deep_sleep(30);
}