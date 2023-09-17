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

//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static uint8_t is_date1_grater_than_date2(struct tm date1, struct tm date2);
static uint8_t is_pwm_in_fading_on_state(struct tm current_time, struct tm turn_on_time);

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
static uint8_t is_pwm_in_fading_on_state(struct tm current_time, struct tm turn_on_time)
{
    if(current_time.tm_hour == turn_on_time.tm_hour)
    {
        if((current_time.tm_min - turn_on_time.tm_min) < 15)
        {
            return 1;
        }
    }
    else if(current_time.tm_hour > turn_on_time.tm_hour)
    {
        if((turn_on_time.tm_min > 45) && (turn_on_time.tm_min < 60))
        {
            if((60 - turn_on_time.tm_min + current_time.tm_min) < 15)
            {
                return 1;
            }
        }
    }
    return 0;
}
//------------------------------------------------------------------------------
static uint8_t is_pwm_in_fading_off_state(struct tm current_time, struct tm turn_off_time)
{
    if(current_time.tm_hour == turn_off_time.tm_hour)
    {
        if((current_time.tm_min - turn_off_time.tm_min) < 15)
        {
            return 1;
        }
    }
    else if(current_time.tm_hour > turn_off_time.tm_hour)
    {
        if((turn_off_time.tm_min > 45) && (turn_off_time.tm_min < 60))
        {
            if((60 - turn_off_time.tm_min + current_time.tm_min) < 15)
            {
                return 1;
            }
        }
    }
    return 0;
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void pwm_auto_manager_handler(pwm_auto_info_t *info, bool pwm_auto_enable)
{
    if(pwm_auto_enable == true)
    {
        if(info->output_status == PWM_OUTPUT_OFF)
        {
            if((is_date1_grater_than_date2(info->current_time, info->turn_on_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_off_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
            {
                #ifdef DEBUG_MODULE
                    printf("PWM_AUTO_WORKING_PWM_ON \n");
                #endif
                info->output_status = PWM_OUTPUT_ON;
                if(info->simul_day_status == true)
                {
                    if(is_pwm_in_fading_on_state(info->current_time, info->turn_on_time))
                    {
                        pwm_manager_turn_on_pwm_simul_day_on(info->percent_power);
                    }
                    else
                    {
                        pwm_manager_turn_on_pwm(info->percent_power);
                    }   
                }
                else
                {
                    pwm_manager_turn_on_pwm(info->percent_power);
                }
            }
            else if((is_date1_grater_than_date2(info->turn_off_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_on_time, info->turn_off_time) == 1))
            {
                #ifdef DEBUG_MODULE
                    printf("PWM_AUTO_WORKING_PWM_ON \n");
                #endif
                info->output_status = PWM_OUTPUT_ON;
                if(info->simul_day_status == true)
                {
                    if(is_pwm_in_fading_on_state(info->current_time, info->turn_on_time))
                    {
                        pwm_manager_turn_on_pwm_simul_day_on(info->percent_power);
                    }
                    else
                    {
                        pwm_manager_turn_on_pwm(info->percent_power);
                    }  
                }
                else
                {
                    pwm_manager_turn_on_pwm(info->percent_power);
                }
            }
        }
        else if(info->output_status == PWM_OUTPUT_ON)
        {
            if((is_date1_grater_than_date2(info->current_time, info->turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
            {
                #ifdef DEBUG_MODULE
                    printf("PWM_AUTO_WORKING_PWM_OFF \n");
                #endif
                info->output_status = PWM_OUTPUT_OFF;
                if(info->simul_day_status == true)
                {
                    if(is_pwm_in_fading_off_state(info->current_time, info->turn_off_time))
                    {
                        pwm_manager_turn_off_pwm_simul_day_on();
                    }
                    else
                    {
                        pwm_manager_turn_off_pwm();
                    }  
                }
                else
                {
                    pwm_manager_turn_off_pwm();
                }
            }
            else if((is_date1_grater_than_date2(info->current_time, info->turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_on_time, info->turn_off_time) == 1))
            {
                #ifdef DEBUG_MODULE
                    printf("PWM_AUTO_WORKING_PWM_OFF \n");
                #endif
                info->output_status = PWM_OUTPUT_OFF;
                if(info->simul_day_status == true)
                {
                    if(is_pwm_in_fading_off_state(info->current_time, info->turn_off_time))
                    {
                        pwm_manager_turn_off_pwm_simul_day_on();
                    }
                    else
                    {
                        pwm_manager_turn_off_pwm();
                    }
                }
                else
                {
                    pwm_manager_turn_off_pwm();
                }
            }
        }
    }
}

//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------