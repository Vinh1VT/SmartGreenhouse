#ifndef DRIVER_SCD41_HPP
#define DRIVER_SCD41_HPP

#include <Wire.h>
#include <DFRobot_SCD4X.h>
#include "sensor_Data.h"

// Create the SCD41 object
extern DFRobot_SCD4X SCD41;

int setup_scd41();
void read_scd41(struct SensorData &data);

#endif