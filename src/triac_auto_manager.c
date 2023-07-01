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

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    TRIAC_AUTO_STANDBY = 1,
    TRIAC_AUTO_WORKING_PWM_OFF_1 = 2,
    TRIAC_AUTO_WORKING_PWM_ON_1 = 3,
    TRIAC_AUTO_WORKING_PWM_OFF_2 = 4,
    TRIAC_AUTO_WORKING_PWM_ON_2 = 5,
    TRIAC_AUTO_WORKING_PWM_OFF_3 = 6,
    TRIAC_AUTO_WORKING_PWM_ON_3 = 7,
    TRIAC_AUTO_WORKING_PWM_OFF_4 = 8,
    TRIAC_AUTO_WORKING_PWM_ON_4 = 9,
}triac_auto_state_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static triac_auto_state_t actual_state;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void triac_auto_start(void)
{
    actual_state = TRIAC_AUTO_WORKING_PWM_OFF_1;
}
//------------------------------------------------------------------------------
void triac_auto_end(void)
{
    actual_state = TRIAC_AUTO_STANDBY;
}
//------------------------------------------------------------------------------
// TO DO: Hay que tener en cuenta el caso en que no finalice el ciclo de forma correcta
// por lo que se deben reconfigurar el calendario automatico para el dia siguiente
// y reiniciarse la maquina de estados para que pueda ser lanzada nuevamente.
void triac_auto_manager_handler(triac_auto_info_t *info)
{
    switch(actual_state)
    {
        case TRIAC_AUTO_STANDBY:
            return;
            break;
        case TRIAC_AUTO_WORKING_PWM_OFF_1:
            if(info->triac_auto[0].enable == true)
            {
                if(info->current_time > info->triac_auto[0].turn_on_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_ON_1;
                    triac_manager_turn_on_triac();
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_2;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_ON_1:
            if(info->triac_auto[0].enable == true)
            {
                if(info->current_time > info->triac_auto[0].turn_off_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_OFF_2;
                    triac_manager_turn_off_triac();
                    info->triac_auto[0].turn_off_time += 86400; 
                    info->triac_auto[0].turn_on_time += 86400; 
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_2;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_OFF_2:
            if(info->triac_auto[1].enable == true)
            {
                if(info->current_time > info->triac_auto[1].turn_on_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_ON_2;
                    triac_manager_turn_on_triac();
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_3;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_ON_2:
            if(info->triac_auto[1].enable == true)
            {
                if(info->current_time > info->triac_auto[1].turn_off_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_OFF_3;
                    triac_manager_turn_off_triac();
                    info->triac_auto[1].turn_off_time += 86400; 
                    info->triac_auto[1].turn_on_time += 86400; 
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_3;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_OFF_3:
            if(info->triac_auto[2].enable == true)
            {
                if(info->current_time > info->triac_auto[2].turn_on_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_ON_3;
                    triac_manager_turn_on_triac();
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_4;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_ON_3:
            if(info->triac_auto[2].enable == true)
            {
                if(info->current_time > info->triac_auto[2].turn_off_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_OFF_4;
                    triac_manager_turn_off_triac();
                    info->triac_auto[2].turn_off_time += 86400; 
                    info->triac_auto[2].turn_on_time += 86400; 
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_4;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_OFF_4:
            if(info->triac_auto[3].enable == true)
            {
                if(info->current_time > info->triac_auto[3].turn_on_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_ON_4;
                    triac_manager_turn_on_triac();
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_1;
            }
            break;
        case TRIAC_AUTO_WORKING_PWM_ON_4:
            if(info->triac_auto[3].enable == true)
            {
                if(info->current_time > info->triac_auto[3].turn_off_time)
                {
                    actual_state = TRIAC_AUTO_WORKING_PWM_OFF_1;
                    triac_manager_turn_off_triac();
                    info->triac_auto[3].turn_off_time += 86400; 
                    info->triac_auto[3].turn_on_time += 86400; 
                }
            }
            else
            {
                actual_state = TRIAC_AUTO_WORKING_PWM_OFF_1;
            }
            break;
        default:
            break;
    }
}

//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------