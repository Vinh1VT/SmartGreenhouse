#include <Wire.h>
#include <Arduino.h>

#define EMC2301_I2C_ADDR 0x2F // Adresse I2C
#define REG_FAN1_SETTING 0x30 // Registre Fan 1 Setting 
#define BUTTON_PIN 4          // Broche du bouton

// int currentSpeedPercent = 0;
// bool lastButtonState = HIGH;

// Fonction qui prend un pourcentage (0-100) et l'envoie au ventilateur
void setFanSpeedPercent(int percentage) {
  // 1. Sécuriser l'entrée pour être sûr de rester entre 0 et 100
  percentage = constrain(percentage, 0, 100);
  
  // 2. Convertir le pourcentage (0-100) en valeur de registre (0-255)
  // Basé sur la formule : Drive = (Value / 255) * 100 
  byte regValue = (percentage * 255) / 100;
  
  // 3. Envoyer la commande via I2C au protocole Write Byte [cite: 900, 901]
  Wire.beginTransmission(EMC2301_I2C_ADDR);
  Wire.write(REG_FAN1_SETTING);
  Wire.write(regValue);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.print("Vitesse définie à : ");
    Serial.print(percentage);
    Serial.print("% (Valeur registre : 0x");
    Serial.print(regValue, HEX);
    Serial.println(")");
  } else {
    Serial.println("Erreur de communication I2C.");
  }
}

void setup_fan() {
  
  // L'I2C est initialisé ici, comme convenu
  // Wire.begin(); 
  
  // Configuration du bouton avec résistance de tirage interne
  // pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // delay(1000);
  // Serial.println("Prêt ! Appuyez sur le bouton (GND sur broche 4) pour incrémenter la vitesse.");
  
  // Initialisation à 0%
  // setFanSpeedPercent(currentSpeedPercent);
}

// void loop() {
//   bool currentButtonState = digitalRead(BUTTON_PIN);

//   // Détection du front descendant (moment où on touche le GND)
//   if (currentButtonState == LOW && lastButtonState == HIGH) {
//     delay(50); // Anti-rebond
    
//     // Vérification que le bouton est toujours enfoncé après le délai
//     if (digitalRead(BUTTON_PIN) == LOW) {
      
//       // Incrément de 5%
//       currentSpeedPercent += 5;
      
//       // Si on dépasse 100%, on recommence à 0%
//       if (currentSpeedPercent > 100) {
//         currentSpeedPercent = 0;
//       }
      
//       // Application de la nouvelle vitesse
//       setFanSpeedPercent(currentSpeedPercent);
//     }
//   }

//   lastButtonState = currentButtonState;
// }