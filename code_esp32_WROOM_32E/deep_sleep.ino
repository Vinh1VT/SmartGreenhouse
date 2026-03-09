#include <Arduino.h>
#include "driver_load_switch.hpp"
#include "deep_sleep.hpp"


void start_deep_sleep(int time) //Temps en seconde
{    
    switch_load(0);
    esp_sleep_enable_timer_wakeup(time*1000000);
    esp_deep_sleep_start();
}