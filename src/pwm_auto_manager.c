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
#include "../include/vege_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static uint8_t pwm_on = false;
static uint8_t pwm_off = false;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void pwm_auto_start(void)
{
    pwm_on = false;
    pwm_off = false;
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void pwm_auto_manager_handler(pwm_auto_info_t *info, bool pwm_auto_enable)
{
    if(pwm_auto_enable == true)
    {
        if((info->current_time.tm_hour == info->turn_on_time.tm_hour) \
        && (info->current_time.tm_min == info->turn_on_time.tm_min)\
        && (info->current_time.tm_sec == 0) && (pwm_on == false))
        {
            #ifdef DEBUG_MODULE
                printf("PWM_AUTO_WORKING_PWM_ON \n");
            #endif
            pwm_on = true;
            pwm_off = false;
            if(info->simul_day_status == true)
            {
                pwm_manager_turn_on_pwm_simul_day_on(info->percent_power);
            }
            else
            {
                pwm_manager_turn_on_pwm(info->percent_power);
            }
        }
        else if((info->current_time.tm_hour == info->turn_off_time.tm_hour) \
            && (info->current_time.tm_min == info->turn_off_time.tm_min)\
            && (info->current_time.tm_sec == 0) && (pwm_off == false))
        {
            #ifdef DEBUG_MODULE
                printf("PWM_AUTO_WORKING_PWM_OFF \n");
            #endif
            pwm_on = false;
            pwm_off = true;
            if(info->simul_day_status == true)
            {
                pwm_manager_turn_off_pwm_simul_day_on();
            }
            else
            {
                pwm_manager_turn_off_pwm();
            }
        }
    }
}

//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------