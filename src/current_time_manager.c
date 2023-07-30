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
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 20
#define SECOND_1 1000
#define UPDATE_TIME 15 // 15 seconds

#define MAX_RETRIES 5
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    SET_CURRENT_TIME = 1,
    SAVE_CURRENT_TIME = 2,
}current_time_cmds_t;

typedef struct{
    struct tm current_time;
    bool read_from_flash;
    current_time_cmds_t cmd;
}current_time_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t current_time_manager_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void current_time_manager_task(void* arg);
static void init_current_time(struct tm* current_time);
static void nv_save_current_time(struct tm current_time);
static void current_time_manager_save_current_time(struct tm current_time);
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
//------------------------------------------------------------------------------
static void nv_init_current_time(void)
{
    struct tm current_time;
    bool read_ok = false;
    uint8_t retries = 0;

    do
    {
        if(read_date_from_flash(CURRENT_TIME_KEY, &current_time))
        {
            read_ok = true;
            #ifdef DEBUG_MODULE
                printf(" CURRENT TIME READ START \n");
                printf(" HOUR: %d \n", current_time.tm_hour);
                printf(" MIN: %d \n", current_time.tm_min);
                printf(" SEC: %d \n", current_time.tm_sec);
                printf(" CURRENT TIME READ END \n");
            #endif
        }
        retries++;
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }while((read_ok != true) && (retries < MAX_RETRIES));
    
    if(read_ok == false)
    {
        #ifdef DEBUG_MODULE
            printf("CURRENT TIME READING FAILED \n");
        #endif
        init_current_time(&current_time);
    }

    current_time_manager_set_current_time(current_time, true);
}
//------------------------------------------------------------------------------
static void nv_save_current_time(struct tm current_time)
{
    write_date_on_flash(CURRENT_TIME_KEY, current_time);
}
//------------------------------------------------------------------------------
static void current_time_manager_save_current_time(struct tm current_time)
{
    current_time_event_t ev;

    ev.cmd = SAVE_CURRENT_TIME;
    ev.current_time = current_time;
    xQueueSend(current_time_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
static void current_time_manager_task(void* arg)
{
    current_time_event_t ev;
    struct tm current_time;
    struct tm *timeinfo;
    time_t current_time_in_sec;
    uint8_t update_time = 0;

    timeinfo = &current_time;

    nv_init_current_time();

    while(1)
    {
        if(xQueueReceive(current_time_manager_queue, &ev, SECOND_1 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case SET_CURRENT_TIME:
                    if(ev.read_from_flash != true)
                    {
                        nv_save_current_time(ev.current_time);
                    }
                    current_time = ev.current_time;
                    break;
                case SAVE_CURRENT_TIME:
                    nv_save_current_time(ev.current_time);
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

            update_time++;
            if(update_time > UPDATE_TIME)
            {
                update_time = 0;
                current_time_manager_save_current_time(current_time);
            }
        }   
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void current_time_manager_init(void)
{
    current_time_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(current_time_event_t));
    
    xTaskCreate(current_time_manager_task, "current_time_manager_task", configMINIMAL_STACK_SIZE*5, 
        NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void current_time_manager_set_current_time(struct tm current_time, bool read_from_flash)
{
    current_time_event_t ev;

    ev.cmd = SET_CURRENT_TIME;
    ev.current_time = current_time;
    ev.read_from_flash = read_from_flash;
    xQueueSend(current_time_manager_queue, &ev, 10);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------