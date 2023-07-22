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
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    TRIAC_AUTO_STANDBY = 1,
    TRIAC_AUTO_WORKING_OFF = 2,
    TRIAC_AUTO_WORKING_ON = 3,
}triac_auto_state_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static triac_auto_state_t actual_state[MAX_TRIAC_CALENDARS];
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index);

//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// TO DO: Hay que tener en cuenta el caso en que no finalice el ciclo de forma correcta
// por lo que se deben reconfigurar el calendario automatico para el dia siguiente
// y reiniciarse la maquina de estados para que pueda ser lanzada nuevamente.
static void triac_auto_manager_handler_per_triac(triac_auto_info_t *info, uint8_t triac_index)
{
    switch(actual_state[triac_index])
    {
        case TRIAC_AUTO_STANDBY:
            return;
            break;
        case TRIAC_AUTO_WORKING_OFF:
            if(info->triac_auto[triac_index].enable == true)
            {
                if((info->current_time.tm_hour > info->triac_auto[triac_index].turn_on_time.tm_hour) \
                    || ((info->current_time.tm_hour == info->triac_auto[triac_index].turn_on_time.tm_hour) \
                    && (info->current_time.tm_min > info->triac_auto[triac_index].turn_on_time.tm_min)))
                {
                    actual_state[triac_index] = TRIAC_AUTO_WORKING_ON;
                    triac_manager_turn_on_triac();
                }
            }
            else
            {
                return;
            }
            break;
        case TRIAC_AUTO_WORKING_ON:
            if(info->triac_auto[triac_index].enable == true)
            {
                if((info->current_time.tm_hour > info->triac_auto[triac_index].turn_off_time.tm_hour) \
                    || ((info->current_time.tm_hour == info->triac_auto[triac_index].turn_off_time.tm_hour) \
                    && (info->current_time.tm_min > info->triac_auto[triac_index].turn_off_time.tm_min)))
                {
                    actual_state[triac_index] = TRIAC_AUTO_WORKING_OFF;
                    triac_manager_turn_off_triac();
                }
            }
            else
            {
                return;
            }
            break;
        default:
            break;
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void triac_auto_start(void)
{
    uint8_t triac_index = 0;

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        actual_state[triac_index] = TRIAC_AUTO_WORKING_OFF;
    }
}
//------------------------------------------------------------------------------
void triac_auto_end(void)
{
    uint8_t triac_index = 0;

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        actual_state[triac_index] = TRIAC_AUTO_STANDBY;
    }
}
//------------------------------------------------------------------------------
void triac_auto_manager_handler(triac_auto_info_t *info)
{
    uint8_t triac_index = 0;

    for(triac_index = 0; triac_index < MAX_TRIAC_CALENDARS; triac_index++)
    {
        triac_auto_manager_handler_per_triac(info, triac_index);
    }
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------