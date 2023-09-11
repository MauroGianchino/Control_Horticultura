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

//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index);
static uint8_t is_date1_grater_than_date2(struct tm date1, struct tm date2);
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
        && (date1.tm_min > date2.tm_min)))
    {
        return 1;
    }
    return 0;
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
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->triac_auto[triac_index].turn_on_time) == 1))
            {
                triac_manager_turn_on_triac();
                info->output_status = TRIAC_OUTPUT_ON;
            }
            else if((is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->triac_auto[triac_index].turn_off_time) == 1))
            {
                triac_manager_turn_on_triac();
                info->output_status = TRIAC_OUTPUT_ON;
            }
        }
        else if(info->output_status == TRIAC_OUTPUT_ON)
        {
            if((is_date1_grater_than_date2(info->current_time, info->triac_auto[triac_index].turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_off_time, info->triac_auto[triac_index].turn_on_time) == 1))
            {
                triac_manager_turn_off_triac();
                info->output_status = TRIAC_OUTPUT_OFF;
            }
            else if((is_date1_grater_than_date2(info->current_time, info->triac_auto[triac_index].turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->triac_auto[triac_index].turn_on_time, info->triac_auto[triac_index].turn_off_time) == 1))
            {
                triac_manager_turn_off_triac();
                info->output_status = TRIAC_OUTPUT_OFF;
            }
        }
    }
    else
    {
        return;
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void triac_auto_manager_handler(triac_auto_info_t *info, bool triac_auto_enable)
{
    uint8_t triac_index = 0;

    if(triac_auto_enable == true)
    {
        for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
        {
            triac_auto_manager_handler_per_triac(info, triac_index);
        }
    }
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------