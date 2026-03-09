#include <Arduino.h>
#include "driver_load_switch.hpp"
#include "deep_sleep.hpp"

#define MULT_US_TO_S 1000000

void start_deep_sleep(int time) //Temps en seconde
{    
    switch_load(0);
    delay(1000);
    esp_sleep_enable_timer_wakeup(time * MULT_US_TO_S);
    esp_deep_sleep_start();
}