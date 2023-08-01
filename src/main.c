#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "../include/wifi_ap.h"
#include "../include/web_server.h"
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

void app_main()
{
  static httpd_handle_t server = NULL;
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
  wifi_init_softap(); // Inicio el AP
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));

  while (true)
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}