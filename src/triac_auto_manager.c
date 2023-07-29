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
static uint8_t triac_on[MAX_TRIAC_CALENDARS] = {0, 0, 0, 0};
static uint8_t triac_off[MAX_TRIAC_CALENDARS] = {0, 0, 0, 0};
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index)
{

    if(info->triac_auto[triac_index].enable == true)
    {
        if((info->current_time.tm_hour == info->triac_auto[triac_index].turn_on_time.tm_hour) \
            && (info->current_time.tm_min == info->triac_auto[triac_index].turn_on_time.tm_min) \
            && (info->current_time.tm_sec == 0) && (triac_on[triac_index] == false))
        {
            #ifdef DEBUG_MODULE
                printf("TRIAC_AUTO_WORKING_TRIAC_ON, TRIAC INDEX: %i \n", (int)triac_index);
            #endif
            triac_on[triac_index] = true;
            triac_off[triac_index] = false;
            triac_manager_turn_on_triac();
        }

        if((info->current_time.tm_hour == info->triac_auto[triac_index].turn_off_time.tm_hour) \
            && (info->current_time.tm_min == info->triac_auto[triac_index].turn_off_time.tm_min) \
            && (info->current_time.tm_sec == 0) && (triac_off[triac_index] == false))
        {
            #ifdef DEBUG_MODULE
                printf("TRIAC_AUTO_WORKING_TRIAC_OFF, TRIAC INDEX: %i \n", (int)triac_index);
            #endif
            triac_on[triac_index] = false;
            triac_off[triac_index] = true;
            triac_manager_turn_off_triac();
        }
    }
    else
    {
        return;
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void triac_auto_start(void)
{
    uint8_t triac_index = 0;

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        triac_on[triac_index] = false;
        triac_off[triac_index] = false;
    }
}
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