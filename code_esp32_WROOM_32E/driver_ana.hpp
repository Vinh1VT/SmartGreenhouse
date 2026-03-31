#ifndef DRIVER_ANA_HPP
#define DRIVER_ANA_HPP 

#include "sensor_Data.h"

void initADC36();
void readADC36(SensorData& data);

#define ANA1_PIN_READ 39
#define ANA_COMMANDE_0 32
#define ANA_COMMANDE_1 33
#define ANA_COMMANDE_2 25

#endif