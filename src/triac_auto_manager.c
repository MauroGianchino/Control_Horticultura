//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "../include/board_def.h"
#include "../include/triac_auto_manager.h"
#include "../include/triac_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define MAX_TRIAC_CALENDARS 4
#define DEBUG_MODULE
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static bool triac_mark_on[4];
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index);
static uint8_t is_date1_grater_than_date2(struct tm date1, struct tm date2);
static void is_first_turn_on_time_greater_than_current_time(triac_auto_info_t *info);
static int is_within_range(struct tm target, struct tm start, struct tm end);
static void must_be_triac_output_off(triac_auto_info_t *info);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static uint8_t is_date1_grater_than_date2(struct tm date1, struct tm date2)
{
    if((date1.tm_hour > date2.tm_hour) \
        || ((date1.tm_hour == date2.tm_hour) \
        && (date1.tm_min > date2.tm_min)) \
        || ((date1.tm_hour == date2.tm_hour) \
        && (date1.tm_min == date2.tm_min) \ 
        && (date1.tm_sec > date2.tm_sec)))
    {
        return 1;
    }
    return 0;
}
//------------------------------------------------------------------------------
static int is_within_range(struct tm target, struct tm start, struct tm end)
{
    if (start.tm_hour < end.tm_hour || 
       (start.tm_hour == end.tm_hour && start.tm_min <= end.tm_min)) {
        // Caso normal (ejemplo: de 9:00 a 17:00)
        if ((target.tm_hour > start.tm_hour || 
            (target.tm_hour == start.tm_hour && target.tm_min >= start.tm_min)) &&
            (target.tm_hour < end.tm_hour || 
            (target.tm_hour == end.tm_hour && target.tm_min <= end.tm_min))) {
            return 1;
        } else {
            return 0;
        }
    } else {
        // Rango cruza la medianoche (ejemplo: de 22:00 a 2:00)
        if ((target.tm_hour > start.tm_hour || 
            (target.tm_hour == start.tm_hour && target.tm_min >= start.tm_min)) ||
            (target.tm_hour < end.tm_hour || 
            (target.tm_hour == end.tm_hour && target.tm_min <= end.tm_min))) {
            return 1;
        } else {
            return 0;
        }
    }
}
//------------------------------------------------------------------------------
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index)
{
    if(info->triac_auto[triac_index].enable == true)
    {
        if(info->output_status == TRIAC_OUTPUT_OFF)
        {
            if((is_date1_grater_than_date2(info->current_time, info->triac_auto[triac_index].turn_on_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->triac_auto[triac_index].turn_on_time) == 1) && (triac_mark_on[triac_index] == false))
            {
                triac_manager_turn_on_triac();
                info->output_status = TRIAC_OUTPUT_ON;
                triac_mark_on[triac_index] = true;
                printf("MARCA 1, TRIAC INDEX: %d \n", triac_index);
            }
            else if((is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->triac_auto[triac_index].turn_off_time) == 1) && (triac_mark_on[triac_index] == false))
            {
                triac_manager_turn_on_triac();
                info->output_status = TRIAC_OUTPUT_ON;
                triac_mark_on[triac_index] = true;
                printf("MARCA 2, TRIAC INDEX: %d \n", triac_index);
            }
        }
        else if(info->output_status == TRIAC_OUTPUT_ON)
        {
            if((is_date1_grater_than_date2(info->current_time, info->triac_auto[triac_index].turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->triac_auto[triac_index].turn_on_time) == 1) && (triac_mark_on[triac_index] == true))
            {
                triac_manager_turn_off_triac();
                info->output_status = TRIAC_OUTPUT_OFF;
                printf("MARCA 3, TRIAC INDEX: %d \n", triac_index);
                triac_mark_on[triac_index] = false;

            }
            else if((is_date1_grater_than_date2(info->current_time, info->triac_auto[triac_index].turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->triac_auto[triac_index].turn_off_time) == 1) && (triac_mark_on[triac_index] == true))
            {
                triac_manager_turn_off_triac();
                info->output_status = TRIAC_OUTPUT_OFF;
                triac_mark_on[triac_index] = false;
                printf("MARCA 4, TRIAC INDEX: %d \n", triac_index);
            }
        }
    }
    else
    {
        return;
    }
}
//------------------------------------------------------------------------------
static void is_first_turn_on_time_greater_than_current_time(triac_auto_info_t *info)
{
    uint8_t triac_index = 0;
    uint8_t index_youngest = 0;
    struct tm ton_youngest;

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        if(info->triac_auto[triac_index].enable == true)
        {
            ton_youngest = info->triac_auto[triac_index].turn_on_time;
            index_youngest = triac_index;
            break;
        }
    }

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        if(info->triac_auto[triac_index].enable == true)
        {
            if(is_date1_grater_than_date2(ton_youngest, info->triac_auto[triac_index].turn_on_time))
            {
                ton_youngest = info->triac_auto[triac_index].turn_on_time;
                index_youngest = triac_index;
            }
        }
    }
    if(info->output_status == TRIAC_OUTPUT_ON)
    {
        if((is_date1_grater_than_date2(ton_youngest, info->current_time) == 1) \
        && (is_date1_grater_than_date2(info->triac_auto[index_youngest].turn_off_time, info->triac_auto[index_youngest].turn_on_time) == 1))
        {
           //printf("ton_youngest: %s \n", asctime(&ton_youngest));
            //printf("Hora actual: %s \n", asctime(&info->current_time));
            info->output_status = TRIAC_OUTPUT_OFF;
            triac_manager_turn_off_triac();
        }
    }
}
//------------------------------------------------------------------------------
static void must_be_triac_output_off(triac_auto_info_t *info)
{
    uint8_t triac_index = 0;
    char out_of_range_mark[MAX_TRIAC_CALENDARS];

    if(info->output_status == TRIAC_OUTPUT_ON)
    {
        for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
        {
            // chequeo si current time no se encuentra dentro de ningun rango
            if(is_within_range(info->current_time, info->triac_auto[triac_index].turn_on_time, info->triac_auto[triac_index].turn_off_time) == 0)
            {
                out_of_range_mark[triac_index] = 1;
            }
        }
        if((out_of_range_mark[0] == 1) && (out_of_range_mark[1] == 1) && (out_of_range_mark[2] == 1) && (out_of_range_mark[3] == 1))
        {
            info->output_status = TRIAC_OUTPUT_OFF;
            triac_manager_turn_off_triac();

            for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
            {
                triac_mark_on[triac_index] = false;
            }
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void triac_auto_manager_init(void)
{
    uint8_t triac_index = 0;
    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        triac_mark_on[triac_index] = false;
    }
}
void triac_auto_manager_update(triac_auto_info_t *info)
{
    uint8_t triac_index = 0;

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        triac_mark_on[triac_index] = false;
    }

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        if(is_within_range(info->current_time, info->triac_auto[triac_index].turn_on_time, info->triac_auto[triac_index].turn_off_time))
        {
            if(info->output_status == TRIAC_OUTPUT_ON)
            {
                triac_mark_on[triac_index] = true;
            } 
        }
        
    }
}
//------------------------------------------------------------------------------
void triac_auto_manager_handler(triac_auto_info_t *info, bool triac_auto_enable)
{
    uint8_t triac_index = 0;

    if(triac_auto_enable == true)
    {
        must_be_triac_output_off(info);


        for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
        {
            triac_auto_manager_handler_per_triac(info, triac_index);
        }
    }
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------