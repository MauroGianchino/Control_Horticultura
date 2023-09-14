#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
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
#include "../include/vege_manager.h"
#include "../include/current_time_manager.h"
#include "../include/wifi_manager.h"
#include "../include/pcf85063.h"

void app_main()
{
  //static httpd_handle_t server = NULL;
  nv_flash_manager_init();
  global_manager_init();
  led_manager_init();
  button_manager_init();
#ifdef ANALOG_POTE
  analog_input_manager_init();
#endif
  pwm_manager_init();
  triac_manager_init();
  vege_manager_init();
  current_time_manager_init();  
  wifi_manager_init();
  pcf85063_init();


  while (true)
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
