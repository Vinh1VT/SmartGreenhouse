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
  
  buffer[i++] = (data.temp_est >> 8) & 0xFF;
  buffer[i++] = data.temp_est & 0xFF;
  
  buffer[i++] = (data.temp_sud >> 8) & 0xFF;
  buffer[i++] = data.temp_sud & 0xFF;
  
  buffer[i++] = (data.temp_ouest >> 8) & 0xFF;
  buffer[i++] = data.temp_ouest & 0xFF;

  // ------------------------------------------------
  // [H] BLOC HUMIDITÉS
  // ------------------------------------------------
  buffer[i++] = 'H';
  buffer[i++] = data.hum_ambiant;
  buffer[i++] = data.hum_est_10;
  buffer[i++] = data.hum_est_30;
  buffer[i++] = data.hum_sud_10;
  buffer[i++] = data.hum_sud_30;
  buffer[i++] = data.hum_ouest_10;
  buffer[i++] = data.hum_ouest_30;

  // ------------------------------------------------
  // [L] BLOC LUMINANCES
  // ------------------------------------------------
  buffer[i++] = 'L';
  buffer[i++] = (data.lum_ambiant >> 8) & 0xFF;
  buffer[i++] = data.lum_ambiant & 0xFF;
  
  buffer[i++] = (data.lum_est >> 8) & 0xFF;
  buffer[i++] = data.lum_est & 0xFF;
  
  buffer[i++] = (data.lum_sud >> 8) & 0xFF;
  buffer[i++] = data.lum_sud & 0xFF;
  
  buffer[i++] = (data.lum_ouest >> 8) & 0xFF;
  buffer[i++] = data.lum_ouest & 0xFF;

  // ------------------------------------------------
  // [G] BLOC GAZ
  // ------------------------------------------------
  buffer[i++] = 'G';
  buffer[i++] = (data.co2 >> 8) & 0xFF;
  buffer[i++] = data.co2 & 0xFF;
  
  buffer[i++] = (data.o2 >> 8) & 0xFF;
  buffer[i++] = data.o2 & 0xFF;

  // ------------------------------------------------
  // [V] BLOC VENTILATEUR
  // ------------------------------------------------
  buffer[i++] = 'V';
  buffer[i++] = data.ventilateur;

  // Retourne la taille exacte de la trame (ici 33 octets)
  return i; 
}


void arrayToHex(uint8_t* buffer, uint8_t length,char* hexString){
  for(int i =0; i<length;i++){
    sprintf(hexString + (i * 2), "%02X", buffer[i]);
  }
}

