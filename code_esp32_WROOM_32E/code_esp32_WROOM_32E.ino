#include <Arduino.h>
#include <cstdio>

#define DEBUG 1

#include "driver_UART.h"
#include "driver_LoRa.h"
#include "driver_ana.hpp"
#include "read_temp_DS18B20.h"
#include "read_temp_and_hum_dht22.h"
#include "sensor_Data.h"
#include "deep_sleep.hpp"
#include "driver_SCD41.hpp"
#include "driver_lux.hpp"

#define MULT_S_TO_MIN 60

#define PAYLOAD_BUFF_LEN 256
#define PAYLOAD_HEXBUFF_LEN PAYLOAD_BUFF_LEN * 2

void print_wakeup_reason(void) {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void setup(void)
{
    switch_load_init();
    delay(500); /* laisse le temps aux périphériques de s'alimenter (jsp si c'est necessaire)*/
    setup_LoRa();
    initADC36();
    setup_scd41();
    setup_lux_etanche();
}

void loop(void)
{
    #if DEBUG == 1
        Serial.printf("\n---------------\n- LOOP START : -\n---------------\n");
    #endif

    print_wakeup_reason();
    
    /* Connecte au reseau LoRa */
    if (connect_LoRa())
    {
        /* connection échoué à voir ce que l'on fait */
        start_deep_sleep(1 * 60);
    }

    #if DEBUG == 1
        print_addr(); /* recupere les adresse one wire des capteurs DS18B20 */
    #endif
    
    /* Recupere les données */
    struct SensorData data;

    readADC36(data);
    read_temperature_ds18b20(data);
    read_scd41(data);
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
    end_com();
    delay(1000);

    #if DEBUG == 1
        Serial.printf("\n------------\n- LOOP STOP -\n------------\n");
        Serial.flush();
    #endif

    /* Lance le deep sleep */
    start_deep_sleep(15);
}