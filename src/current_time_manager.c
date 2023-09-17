//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
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
#include "../include/nv_flash_manager.h"
#include "../include/pcf85063.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 10
#define SECOND_1 1000
#define UPDATE_TIME 15 // 15 seconds

#define MAX_RETRIES 5
#define TIMEOUT_MS 500
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    SET_CURRENT_TIME = 1,
    GET_CURRENT_TIME = 2,
}current_time_cmds_t;

typedef struct{
    struct tm current_time;
    current_time_cmds_t cmd;
}current_time_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t current_time_manager_queue, response_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void current_time_manager_task(void* arg);
static void get_current_time(void);
static uint8_t wait_get_current_time_response(struct tm *current_time);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void current_time_manager_task(void* arg)
{
    current_time_event_t ev;
    struct tm current_time;

    pcf85063_init();
    
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    while(1)
    {
        if(xQueueReceive(current_time_manager_queue, &ev, SECOND_1 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case SET_CURRENT_TIME:
                    pcf85063_set_current_time(ev.current_time);
                    break;
                case GET_CURRENT_TIME:
                    pcf85063_get_current_time_info(&ev.current_time);
                    xQueueSend(response_queue, &ev, 10);
                    break;
                default:
                    break;
            }
        }
        else
        {
            pcf85063_get_current_time_info(&current_time);
            
            global_manager_update_current_time(current_time);
        }   
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void current_time_manager_init(void)
{
    current_time_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(current_time_event_t));
    response_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(current_time_event_t));

    xTaskCreate(current_time_manager_task, "current_time_manager_task", configMINIMAL_STACK_SIZE*7, 
        NULL, configMAX_PRIORITIES-5, NULL);
}
//------------------------------------------------------------------------------
void current_time_manager_set_current_time(struct tm current_time)
{
    current_time_event_t ev;

    ev.cmd = SET_CURRENT_TIME;
    ev.current_time = current_time;
    xQueueSend(current_time_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
static void get_current_time(void)
{
    current_time_event_t ev;
    ev.cmd = GET_CURRENT_TIME;
    xQueueSend(current_time_manager_queue, &ev, 10);
}

static uint8_t wait_get_current_time_response(struct tm *current_time) 
{
    current_time_event_t resp_ev;
    if(xQueueReceive(response_queue, &resp_ev, TIMEOUT_MS / portTICK_PERIOD_MS)) 
    {
        *current_time = resp_ev.current_time;
        return 1;
    } 
    else 
    {
        #ifdef DEBUG_MODULE
            printf("Error al recibir la respuesta de pwm\n");
        #endif
        return 0;
    }
    return 0;
}

uint8_t global_manager_get_current_time_info(struct tm *current_time)
{
    get_current_time();
    if(wait_get_current_time_response(current_time))
    {
        return(1);
    }
    return(0);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------