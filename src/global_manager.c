//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 20
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,

}global_event_cmds_t;

typedef struct{
    global_event_cmds_t cmd;
}global_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t global_manager_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void global_manager_task(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void global_manager_task(void* arg)
{
    global_event_t global_ev;

    while(1)
    {
        if(xQueueReceive(global_manager_queue, &global_ev, portMAX_DELAY ) == pdTRUE)
        {
            switch(global_ev.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                default:
                    break;
            }
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void global_manager_init(void)
{
    global_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(global_event_t));
    
    xTaskCreate(global_manager_task, "global_manager_task", configMINIMAL_STACK_SIZE*2, 
        NULL, configMAX_PRIORITIES-1, NULL);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------