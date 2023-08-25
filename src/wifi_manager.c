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
typedef enum{
    CMD_UNDEFINED,
    SET_SSID,
    SET_PASSWORD,
}cmds_t;

typedef struct{
    cmds_t cmd;
    char str_value[80];
}wifi_maanger_events_t;
//--------------------DECLARACION DE DATOS INTERNOS-----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t wifi_manager_queue;
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
    wifi_maanger_events_t ev;
    char ssid[64];
    char password[64];
    bool set_ssid = false;
    bool set_password = false;
    static httpd_handle_t server = NULL;

    while(true)
    {
        if(xQueueReceive(wifi_manager_queue, &ev, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case SET_SSID:
                    strcpy(ssid, ev.str_value);
                    set_ssid = true;
                    break;
                case SET_PASSWORD:
                    strcpy(password, ev.str_value);
                    set_password = true;
                    break;
                default:
                break;
            }
        }
        else
        {
            if((set_ssid == true) && (set_password == true))
            {
                set_ssid = false;
                set_password = false;

                wifi_init_softap(); // Inicio el AP
                ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));
            }
        }
    }
}
//--------------------DEFINICION DE FUNCIONES EXTERNAS--------------------------
//------------------------------------------------------------------------------
void wifi_manager_init(void)
{
    wifi_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(wifi_maanger_events_t));

    xTaskCreate(wifi_manager_task, "wifi_manager_task", 
               configMINIMAL_STACK_SIZE*5, NULL, configMAX_PRIORITIES, NULL);             
}
//--------------------FIN DEL ARCHIVO-------------------------------------------
//------------------------------------------------------------------------------