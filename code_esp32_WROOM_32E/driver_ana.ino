#include <Arduino.h>
#include "driver_ana.hpp"
#include "sensor_Data.h"


#include "sensor_Data.h"

float ANA1_CAPA[8];

void initADC36(void)
{
  analogReadResolution(12);          // 12 bits → 0 à 4095
  analogSetAttenuation(ADC_11db);    // Plage ~0 à 3.3V
  

  pinMode(ANA_COMMANDE_0, OUTPUT);
  pinMode(ANA_COMMANDE_1, OUTPUT);
  pinMode(ANA_COMMANDE_2, OUTPUT);
}

static void getADC36(void)
{


  for(int i = 0; i < 8; i++) {

    int A = i & 1;
    int B = (i >> 1) & 1;
    int C = (i >> 2) & 1;

    digitalWrite(ANA_COMMANDE_0, A);
    digitalWrite(ANA_COMMANDE_1, B);
    digitalWrite(ANA_COMMANDE_2, C);


    delay(100);

    int analogValue = analogRead(ANA1_PIN_READ);
    ANA1_CAPA[i] = analogValue * (3.3 / 4095.0);

    float voltage = analogValue * (3.3 / 4095.0);

    #if DEBUG == 1
    Serial.print("Canal ");
    Serial.print(i);
    Serial.print(" -> ");
    Serial.print(voltage);
    Serial.println(" V");
    #endif

  }

  
    digitalWrite(ANA_COMMANDE_0, 0);
    digitalWrite(ANA_COMMANDE_1, 0);
    digitalWrite(ANA_COMMANDE_2, 0);
}

/* temporaire, il faudra etaloner les capteur plus tard */
static uint8_t convert_VOLT_to_hum(float t)
{
  return (uint8_t) ((((1./3.3) * -t) + 1.) * 100.);
}

void readADC36(SensorData& data)
{
  getADC36();
  data.hum_oya_1 = convert_VOLT_to_hum(ANA1_CAPA[0]);
  data.hum_oya_2 = convert_VOLT_to_hum(ANA1_CAPA[1]);
  data.hum_oya_3 = convert_VOLT_to_hum(ANA1_CAPA[2]);
  data.hum_oya_4 = convert_VOLT_to_hum(ANA1_CAPA[3]);
  data.hum_bille_1 = convert_VOLT_to_hum(ANA1_CAPA[4]);
  data.hum_bille_2 = convert_VOLT_to_hum(ANA1_CAPA[5]);
}