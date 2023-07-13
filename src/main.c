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
#include "../include/nv_flash_manager.h"
#include "../include/triac_manager.h"

void app_main() 
{
  nv_flash_manager_init();
  global_manager_init();
  led_manager_init();
  button_manager_init();
  #ifdef ANALOG_POTE
    analog_input_manager_init();
  #endif    
  pwm_manager_init();
  triac_manager_init();

  while(true) 
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}