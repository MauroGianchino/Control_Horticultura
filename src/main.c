#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/led_manager.h"

void app_main() 
{
    led_manager_init();
    
    while(true) 
    {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}