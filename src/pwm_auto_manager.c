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
#include "../include/pwm_auto_manager.h"
#include "../include/pwm_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    PWM_AUTO_STANDBY = 1,
    PWM_AUTO_WORKING_PWM_OFF = 2,
    PWM_AUTO_WORKING_PWM_ON = 3,
}pwm_auto_state_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static pwm_auto_state_t actual_state;
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
void pwm_auto_start(void)
{
    actual_state = PWM_AUTO_WORKING_PWM_OFF;
}
//------------------------------------------------------------------------------
void pwm_auto_end(void)
{
    actual_state = PWM_AUTO_STANDBY;
}
//------------------------------------------------------------------------------
// TO DO: Hay que tener en cuenta el caso en que no finalice el ciclo de forma correcta
// por lo que se deben reconfigurar el calendario automatico para el dia siguiente
// y reiniciarse la maquina de estados para que pueda ser lanzada nuevamente.
void pwm_auto_manager_handler(pwm_auto_info_t *info)
{
    switch(actual_state)
    {
        case PWM_AUTO_STANDBY:
            return;
            break;
        case PWM_AUTO_WORKING_PWM_OFF:
            if((info->current_time.tm_hour > info->turn_on_time.tm_hour) \
                || ((info->current_time.tm_hour == info->turn_on_time.tm_hour) \
                && (info->current_time.tm_min > info->turn_on_time.tm_min)))
            {
                actual_state = PWM_AUTO_WORKING_PWM_ON;
                if(info->simul_day_status == true)
                {
                    pwm_manager_turn_on_pwm_simul_day_on(info->percent_power);
                }
                else
                {
                    pwm_manager_turn_on_pwm(info->percent_power);
                }
            }
            break;
        case PWM_AUTO_WORKING_PWM_ON:
            if((info->current_time.tm_hour > info->turn_off_time.tm_hour) \
                || ((info->current_time.tm_hour == info->turn_off_time.tm_hour) \
                && (info->current_time.tm_min > info->turn_off_time.tm_min)))
            {
                actual_state = PWM_AUTO_WORKING_PWM_OFF;
                if(info->simul_day_status == true)
                {
                    pwm_manager_turn_off_pwm_simul_day_on();
                }
                else
                {
                    pwm_manager_turn_off_pwm();
                }
            }
            break;
        default:
            break;
    }
}

//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------