#include <Arduino.h>
#include "driver_ana.hpp"

float ANA1_CAPA[8];

void initADC36() {
  analogReadResolution(12);          // 12 bits → 0 à 4095
  analogSetAttenuation(ADC_11db);    // Plage ~0 à 3.3V
  

  pinMode(ANA_COMMANDE_0, OUTPUT);
  pinMode(ANA_COMMANDE_1, OUTPUT);
  pinMode(ANA_COMMANDE_2, OUTPUT);
}
void readADC36() {


  for(int i = 0; i < 8; i++) {

    int A = i & 1;
    int B = (i >> 1) & 1;
    int C = (i >> 2) & 1;

    digitalWrite(ANA_COMMANDE_0, A);
    digitalWrite(ANA_COMMANDE_1, B);
    digitalWrite(ANA_COMMANDE_2, C);


    delay(1000);

    int analogValue = analogRead(ANA1_PIN_READ);
    ANA1_CAPA[i] = analogValue * (3.3 / 4095.0);

    float voltage = analogValue * (3.3 / 4095.0);

    Serial.print("Canal ");
    Serial.print(i);
    Serial.print(" -> ");
    Serial.print(voltage);
    Serial.println(" V");

  }

  
    digitalWrite(ANA_COMMANDE_0, 0);
    digitalWrite(ANA_COMMANDE_1, 0);
    digitalWrite(ANA_COMMANDE_2, 0);
}