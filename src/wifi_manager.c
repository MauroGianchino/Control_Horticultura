//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

#include "../include/wifi_ap.h"
#include "../include/web_server.h"
#include "../include/wifi_manager.h"
#include "../include/board_def.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define QUEUE_ELEMENT_QUANTITY 20

//#define DEBUG_MODULE
//------------------------------TYPEDEF-----------------------------------------
//------------------------------------------------------------------------------

//--------------------DECLARACION DE DATOS INTERNOS-----------------------------
//------------------------------------------------------------------------------
//static QueueHandle_t wifi_manager_queue;
//--------------------DECLARACION DE FUNCIONES INTERNAS-------------------------
//------------------------------------------------------------------------------
static void wifi_manager_task(void * pvParameters);
//--------------------DEFINICION DE DATOS INTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE DATOS EXTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE FUNCIONES INTERNAS--------------------------
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void wifi_manager_task(void * pvParameters)
{
    //wifi_maanger_events_t ev;
    static httpd_handle_t server = NULL;

    vTaskDelay(100 / portTICK_PERIOD_MS);

    wifi_init_softap(); // Inicio el AP
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));

    while(true)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
//--------------------DEFINICION DE FUNCIONES EXTERNAS--------------------------
//------------------------------------------------------------------------------
void wifi_manager_init(void)
{
   // wifi_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(wifi_maanger_events_t));

    xTaskCreate(wifi_manager_task, "wifi_manager_task", 
               configMINIMAL_STACK_SIZE*10, NULL, configMAX_PRIORITIES-1, NULL);             
}
//--------------------FIN DEL ARCHIVO-------------------------------------------
//------------------------------------------------------------------------------