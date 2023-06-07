#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/led_manager.h"
#include "../include/button_manager.h"
#include "../include/global_manager.h"

void app_main() 
{
  global_manager_init();
  led_manager_init();
  button_manager_init();
      
  while(true) 
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}