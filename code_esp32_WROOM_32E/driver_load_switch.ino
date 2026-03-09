#include <Arduino.h>
#include "driver_load_switch.hpp"
#include "sensor_Data.h"


void switch_load_init(){
  pinMode(PATE_LOAD, OUTPUT);
  switch_load(1);
}
void switch_load(int etat){
    switch(etat){
        case 0:
        digitalWrite(PATE_LOAD, 0);
        break;
        case 1:
        digitalWrite(PATE_LOAD, 1);
        break;
        default:
        Serial.printf("Etat different de 0 ou 1");
        break;
    }
}