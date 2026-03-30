#ifndef DRIVER_IA_H
#define DRIVER_IA_H

#include "sensor_Data.h"

#define PATE_INTERRUPT_IA 19
#define INTERRUPT_LEVEL 0
#define ADRESSE_CARTE 0x08
#define LONGUEUR_REPONSE 1 //en octet

void init_carte_IA();
bool com_IA_I2C(SensorData& data);


#endif
