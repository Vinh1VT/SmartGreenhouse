#include <Arduino.h>
#include "sensor_Data.h"


// Fonction qui remplit le buffer et retourne sa taille finale
uint8_t buildPayload(const SensorData& data, uint8_t* buffer) {
  uint8_t i = 0; // Index actuel dans le tableau

  // ------------------------------------------------
  // [T] BLOC TEMPÉRATURES
  // ------------------------------------------------
  buffer[i++] = 'T';
  buffer[i++] = (data.temp_ambiant >> 8) & 0xFF; // Octet de poids fort
  buffer[i++] = data.temp_ambiant & 0xFF;        // Octet de poids faible
  
  buffer[i++] = (data.temp_puit_out >> 8) & 0xFF;
  buffer[i++] = data.temp_puit_out & 0xFF;
  
  buffer[i++] = (data.temp_puit_in >> 8) & 0xFF;
  buffer[i++] = data.temp_puit_in & 0xFF;
  
  buffer[i++] = (data.temp_terre_1 >> 8) & 0xFF;
  buffer[i++] = data.temp_terre_1 & 0xFF;

  buffer[i++] = (data.temp_terre_2 >> 8) & 0xFF;
  buffer[i++] = data.temp_terre_2 & 0xFF;

  // ------------------------------------------------
  // [H] BLOC HUMIDITÉS
  // ------------------------------------------------
  buffer[i++] = 'H';
  buffer[i++] = data.hum_ambiant;
  buffer[i++] = data.hum_oya_1;
  buffer[i++] = data.hum_oya_2;
  buffer[i++] = data.hum_oya_3;
  buffer[i++] = data.hum_oya_4;
  buffer[i++] = data.hum_bille_1;
  buffer[i++] = data.hum_bille_2;

  // ------------------------------------------------
  // [L] BLOC LUMINANCES
  // ------------------------------------------------
  buffer[i++] = 'L';
  buffer[i++] = (data.lum_ambiant >> 8) & 0xFF;
  buffer[i++] = data.lum_ambiant & 0xFF;

  // ------------------------------------------------
  // [G] BLOC GAZ
  // ------------------------------------------------
  buffer[i++] = 'G';
  buffer[i++] = (data.co2 >> 8) & 0xFF;
  buffer[i++] = data.co2 & 0xFF;
  
  buffer[i++] = (data.o2 >> 8) & 0xFF;
  buffer[i++] = data.o2 & 0xFF;

  // Retourne la taille exacte de la trame 
  return i; 
}


void arrayToHex(uint8_t* buffer, uint8_t length, char* hexString){
  for(int i =0; i<length;i++){
    sprintf(hexString + (i * 2), "%02X", buffer[i]);
  }
}

