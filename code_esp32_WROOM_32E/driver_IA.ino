#include "driver_IA.hpp"
#include "sensor_Data.h"


void writeReg(uint8_t address, uint8_t reg, uint8_t value);



void init_carte_IA(){
    pinMode(PATE_INTERRUPT_IA, OUTPUT);
}


bool com_IA_I2C(SensorData& data){
    digitalWrite(PATE_INTERRUPT_IA, INTERRUPT_LEVEL);
    delay(100);
    digitalWrite(PATE_INTERRUPT_IA, INTERRUPT_LEVEL^1);

    // On demande de lancer la photo etc
    writeReg(ADRESSE_CARTE,0,0xCA);
    delay(1000);

    // On demande d'envoyer le resultat
    writeReg(ADRESSE_CARTE,0,0xCA);
    delay(1000);

    int data_buff[8]:
    // Request data
    uint8_t received = Wire.requestFrom(ADRESSE_CARTE, LONGUEUR_REPONSE);
    if (received != len)
        return false;

    for (uint8_t i = 0; i < LONGUEUR_REPONSE; i++) {
        data_buff[i] = Wire.read();
    }
    int message = 0;
    for(int i = 0; i<ADRESSE_CARTE;i++){
        message += 1<<i;
    }
    Serial.print("\nRéponse modèle : ")
    Serial.println(message);
    return true;

}