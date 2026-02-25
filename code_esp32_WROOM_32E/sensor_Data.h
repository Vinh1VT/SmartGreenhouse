#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <Arduino.h>

struct SensorData{
  // --- Bloc T: Températures (int16_t) ---
  // Défaut : 32766 (0x7FFE) -> Absent
  // 32767 (0x7FFF) -> ERREUR CAPTEUR
  int16_t temp_ambiant = 0x7FFE;
  int16_t temp_est     = 0x7FFE;
  int16_t temp_sud     = 0x7FFE;
  int16_t temp_ouest   = 0x7FFE;

  // --- Bloc H: Humidités (uint8_t) ---
  // Défaut : 254 (0xFE) -> Absent
  // 255 (0xFF) -> ERREUR CAPTEUR
  uint8_t hum_ambiant  = 0xFE;
  uint8_t hum_est_10   = 0xFE;
  uint8_t hum_est_30   = 0xFE;
  uint8_t hum_sud_10   = 0xFE;
  uint8_t hum_sud_30   = 0xFE;
  uint8_t hum_ouest_10 = 0xFE;
  uint8_t hum_ouest_30 = 0xFE;

  // --- Bloc L: Luminances (uint16_t) ---
  // Défaut : 65534 (0xFFFE) -> Absent
  // 65535 (0xFFFF) -> ERREUR CAPTEUR
  uint16_t lum_ambiant = 0xFFFE;
  uint16_t lum_est     = 0xFFFE;
  uint16_t lum_sud     = 0xFFFE;
  uint16_t lum_ouest   = 0xFFFE;

  // --- Bloc G: Gaz (uint16_t) ---
  // Défaut : 65534 (0xFFFE) -> Absent
  // 65535 (0xFFFF) -> ERREUR CAPTEUR
  uint16_t co2         = 0xFFFE;
  uint16_t o2          = 0xFFFE;

  // --- Bloc V: Ventilateur (uint8_t) ---
  // Défaut : 254 (0xFE) -> Absent (si on veut omettre le bloc)
  // 255 (0xFF) -> Erreur Capteur 
  uint8_t ventilateur  = 0xFE; 
};




#endif