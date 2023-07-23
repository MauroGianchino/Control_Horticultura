//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "../include/current_time_manager.h"
#include "../include/board_def.h"
#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 20
#define SECOND_1 1000
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
}current_time_cmds_t;

typedef struct{
    current_time_cmds_t cmd;
}current_time_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t current_time_manager_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void current_time_manager_task(void* arg);
static void init_current_time(struct tm* current_time);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void init_current_time(struct tm* current_time)
{
    current_time->tm_year = 123; // anios desde 1900
    current_time->tm_mon = 7; // 0-11
    current_time->tm_mday = 23; // 1-31
    current_time->tm_hour = 13; // 0-23
    current_time->tm_min = 0; // 0-59
    current_time->tm_sec = 0; // 0-59
}
//------------------------------------------------------------------------------
static void current_time_manager_task(void* arg)
{
    current_time_event_t ev;
    struct tm current_time;
    struct tm *timeinfo;
    time_t current_time_in_sec;

    timeinfo = &current_time;

    init_current_time(&current_time);
    while(1)
    {
        if(xQueueReceive(current_time_manager_queue, &ev, SECOND_1 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                default:
                    break;
            }
        }
        else
        {
            current_time_in_sec = mktime(&current_time);
            current_time_in_sec++;
            *timeinfo = *localtime(&current_time_in_sec);
        
            global_manager_update_current_time(current_time);
        }   
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void current_time_manager_init(void)
{
    current_time_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(current_time_event_t));
    
    xTaskCreate(current_time_manager_task, "current_time_manager_task", configMINIMAL_STACK_SIZE*2, 
        NULL, configMAX_PRIORITIES-2, NULL);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------