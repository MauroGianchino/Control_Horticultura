#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/led_manager.h"
#include "../include/button_manager.h"
#include "../include/global_manager.h"
#include "../include/board_def.h"
#include "../include/pwm_manager.h"
#ifdef ANALOG_POTE
  #include "../include/analog_input_manager.h"
#endif

void app_main() 
{
  global_manager_init();
  led_manager_init();
  button_manager_init();
  #ifdef ANALOG_POTE
    analog_input_manager_init();
  #endif    
  pwm_manager_init();
  while(true) 
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}