#include <Arduino.h>

void initADC36() {
  analogReadResolution(12);          // 12 bits → 0 à 4095
  analogSetAttenuation(ADC_11db);    // Plage ~0 à 3.3V
}

void readADC36() {
  const int analogPin = 36;  // GPIO36 = ADC1_CHANNEL_0
  
  int analogValue = analogRead(analogPin);
  const int commandeA = 0;
  const int commandeB = 2;
  const int commandeC = 15;

    pinMode(commandeA, OUTPUT);
    pinMode(commandeB, OUTPUT);
    pinMode(commandeC, OUTPUT);

  int A,B,C;

  for(int i = 0; i<8;i++){
    A = i&1;
    B = (i>>1)&1;
    C = (i>>2)&1;

    Serial.print("\n");
    Serial.print(A);
    Serial.print(B);
    Serial.print(C);

    Serial.print("\n");

    digitalWrite(commandeA, A);
    digitalWrite(commandeB, B);
    digitalWrite(commandeC, C);
    Serial.print("Pin n° :");
    Serial.print(i);
    Serial.print("Valeur analogique : ");
    Serial.print(analogValue);

    float voltage = analogValue * (3.3 / 4095.0);
    Serial.print(" | Tension : ");
    Serial.print(voltage);
    Serial.println(" V");
    
    delay(1000);

  }

}