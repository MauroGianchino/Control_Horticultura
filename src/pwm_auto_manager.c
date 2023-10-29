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
#include "../include/led_manager.h"
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
static void subtract_15_minutes(struct tm *t); 
static int is_within_range(struct tm target, struct tm start, struct tm end);

//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void subtract_15_minutes(struct tm *t) 
{
    time_t rawtime = mktime(t); // Convierte struct tm a time_t (segundos desde epoch)
    rawtime -= 900; // Resta 900 segundos (15 minutos)
    *t = *localtime(&rawtime); // Convierte de nuevo a struct tm
}
//------------------------------------------------------------------------------
static int is_within_range(struct tm target, struct tm start, struct tm end)
{
    if (start.tm_hour < end.tm_hour || 
       (start.tm_hour == end.tm_hour && start.tm_min < end.tm_min) ||
       (start.tm_hour == end.tm_hour && start.tm_min == end.tm_min && start.tm_sec <= end.tm_sec)) {
        // Caso normal (ejemplo: de 9:00:00 a 17:00:00)
        if ((target.tm_hour > start.tm_hour || 
            (target.tm_hour == start.tm_hour && target.tm_min > start.tm_min) ||
            (target.tm_hour == start.tm_hour && target.tm_min == start.tm_min && target.tm_sec >= start.tm_sec)) &&
            (target.tm_hour < end.tm_hour || 
            (target.tm_hour == end.tm_hour && target.tm_min < end.tm_min) ||
            (target.tm_hour == end.tm_hour && target.tm_min == end.tm_min && target.tm_sec <= end.tm_sec))) {
            return 1;
        } else {
            return 0;
        }
    } else {
        // Rango cruza la medianoche (ejemplo: de 22:00:00 a 2:00:00)
        if ((target.tm_hour > start.tm_hour || 
            (target.tm_hour == start.tm_hour && target.tm_min > start.tm_min) ||
            (target.tm_hour == start.tm_hour && target.tm_min == start.tm_min && target.tm_sec >= start.tm_sec)) ||
            (target.tm_hour < end.tm_hour || 
            (target.tm_hour == end.tm_hour && target.tm_min < end.tm_min) ||
            (target.tm_hour == end.tm_hour && target.tm_min == end.tm_min && target.tm_sec <= end.tm_sec))) {
            return 1;
        } else {
            return 0;
        }
    }
}
//------------------------------------------------------------------------------
static uint8_t is_date1_grater_than_date2(struct tm date1, struct tm date2)
{
    if((date1.tm_hour > date2.tm_hour) \
        || ((date1.tm_hour == date2.tm_hour) \
        && (date1.tm_min > date2.tm_min)) \
        || ((date1.tm_hour == date2.tm_hour) \
        && (date1.tm_min == date2.tm_min) && (date1.tm_sec > date2.tm_sec)))
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
static bool is_fading_off_started = false;

void turn_off_fading_status(void)
{
    is_fading_off_started = false;
}

void pwm_auto_manager_handler(pwm_auto_info_t *info, bool pwm_auto_enable)
{
    struct tm toff_aux;
    static uint8_t pwm_percentage_ant = 10;
    if(pwm_auto_enable == true)
    {
        if(info->output_status == PWM_OUTPUT_OFF)
        {
            if(info->update_output_percent_power == true)
            {
                info->update_output_percent_power = false;
            }
            else if(is_within_range(info->current_time, info->turn_on_time, info->turn_off_time))
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
                        led_manager_send_pwm_info(info->percent_power, 1, true);
                    }
                    else
                    {
                        pwm_manager_turn_on_pwm(info->percent_power);
                        led_manager_send_pwm_info(info->percent_power, 0, false);
                    }  
                }
                else
                {
                    pwm_manager_turn_on_pwm(info->percent_power);
                    led_manager_send_pwm_info(info->percent_power, 0, false);
                }
            }
        }
        else if(info->output_status == PWM_OUTPUT_ON)
        {
            if(info->simul_day_status == true) 
            {
                if(is_within_range(info->current_time, info->turn_on_time, info->turn_off_time))
                {
                    if(info->update_calendar == true)
                    {
                        toff_aux = info->turn_off_time;
                        subtract_15_minutes(&toff_aux);

                        info->update_calendar = false;
                        if(is_pwm_in_fading_on_state(info->current_time, info->turn_on_time))
                        {
                            pwm_manager_turn_on_pwm_simul_day_on(info->percent_power);
                            led_manager_send_pwm_info(info->percent_power, 1, true);
                        }
                        else if(is_pwm_in_fading_off_state(info->current_time, toff_aux)) 
                        {
                            pwm_manager_turn_off_pwm_simul_day_on(info->percent_power);
                            is_fading_off_started = true;
                            led_manager_send_pwm_info(info->percent_power, 1, true);
                        }
                        else
                        {
                            pwm_manager_turn_on_pwm(info->percent_power);
                            led_manager_send_pwm_info(info->percent_power, 0, false);
                        }
                    }

                    if(is_fading_off_started == false)
                    {
                        toff_aux = info->turn_off_time;
                        subtract_15_minutes(&toff_aux); 

                        if((pwm_percentage_ant != info->percent_power) && (!is_fading_in_progress()))
                        {
                            #ifdef DEBUG_MODULE
                                printf("PWM_AUTO_POWER UPDATED \n");
                            #endif
                            pwm_manager_turn_on_pwm(info->percent_power);
                            led_manager_send_pwm_info(info->percent_power, 0, false);
                        }

                        if((info->update_output_percent_power == true) && (!is_fading_in_progress()))
                        {
                            info->update_output_percent_power = false;
                            #ifdef DEBUG_MODULE
                                printf("PWM_AUTO_POWER UPDATED \n");
                            #endif

                            pwm_manager_turn_on_pwm(info->percent_power);
                            led_manager_send_pwm_info(info->percent_power, 1, true);
                        }

                        if((is_date1_grater_than_date2(info->current_time, toff_aux) == 1) \
                        && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
                        {
                            #ifdef DEBUG_MODULE
                                printf("PWM_AUTO_WORKING_PWM_OFF \n");
                            #endif
                            if(is_pwm_in_fading_off_state(info->current_time, toff_aux))
                            {
                                pwm_manager_turn_off_pwm_simul_day_on(info->percent_power);
                                is_fading_off_started = true;
                                led_manager_send_pwm_info(info->percent_power, 1, true);
                            }
                            else
                            {
                                pwm_manager_turn_off_pwm();
                                led_manager_send_pwm_info(0, 0, false);
                                info->output_status = PWM_OUTPUT_OFF;
                            }  
                        }
                        else if((is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                            && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
                        {
                            info->output_status = PWM_OUTPUT_OFF;
                            pwm_manager_turn_off_pwm();
                            led_manager_send_pwm_info(0, 0, false);
                        }
                        else if((is_date1_grater_than_date2(info->current_time, toff_aux) == 1) \
                            && (is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                            && (is_date1_grater_than_date2(info->turn_on_time, info->turn_off_time) == 1))
                        {
                            #ifdef DEBUG_MODULE
                                printf("PWM_AUTO_WORKING_PWM_OFF \n");
                            #endif
                            
                            if(is_pwm_in_fading_off_state(info->current_time, toff_aux))
                            {
                                pwm_manager_turn_off_pwm_simul_day_on(info->percent_power);
                                is_fading_off_started = true;
                                led_manager_send_pwm_info(info->percent_power, 1, true);
                            }
                            else
                            {
                                info->output_status = PWM_OUTPUT_OFF;
                                pwm_manager_turn_off_pwm();
                                led_manager_send_pwm_info(0, 0, false);
                            }
                        }
                    }
                    else
                    {
                        if((is_date1_grater_than_date2(info->current_time, info->turn_off_time) == 1) \
                            && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
                        {
                            #ifdef DEBUG_MODULE
                                printf("PWM_AUTO_WORKING_PWM_OFF Toff mayor Ton \n");
                            #endif
                            info->output_status = PWM_OUTPUT_OFF;
                            is_fading_off_started = false;
                            pwm_manager_turn_off_pwm();
                            led_manager_send_pwm_info(0, 0, false);
                        }
                        else if((is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                            && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
                        {
                            info->output_status = PWM_OUTPUT_OFF;
                            pwm_manager_turn_off_pwm();
                            is_fading_off_started = false;
                            led_manager_send_pwm_info(0, 0, false);
                        }
                        else if((is_date1_grater_than_date2(info->current_time, info->turn_off_time) == 1) \
                            && (is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                            && (is_date1_grater_than_date2(info->turn_on_time, info->turn_off_time) == 1))
                        {
                            #ifdef DEBUG_MODULE
                                printf("PWM_AUTO_WORKING_PWM_OFF Ton mayor Toff \n");
                            #endif
                            info->output_status = PWM_OUTPUT_OFF;
                            pwm_manager_turn_off_pwm();
                            is_fading_off_started = false;
                            led_manager_send_pwm_info(0, 0, false);
                            
                        }
                    }
                }   
                else
                {
                    #ifdef DEBUG_MODULE
                        printf("PWM_AUTO_WORKING_PWM_OFF Ton mayor Toff \n");
                    #endif
                    info->output_status = PWM_OUTPUT_OFF;
                    pwm_manager_turn_off_pwm();
                    is_fading_off_started = false;
                    led_manager_send_pwm_info(0, 0, false);
                }
            }
            else
            {
                if((pwm_percentage_ant != info->percent_power) && (!is_fading_in_progress()))
                {
                    #ifdef DEBUG_MODULE
                        printf("PWM_AUTO_POWER UPDATED \n");
                    #endif
                    pwm_manager_turn_on_pwm(info->percent_power);
                    led_manager_send_pwm_info(info->percent_power, 0, false);
                }

                if((info->update_output_percent_power == true) && (!is_fading_in_progress()))
                {
                    info->update_output_percent_power = false;
                    #ifdef DEBUG_MODULE
                        printf("PWM_AUTO_POWER UPDATED \n");
                    #endif
                    pwm_manager_turn_on_pwm(info->percent_power);
                    led_manager_send_pwm_info(info->percent_power, 0, false);
                }


                if((is_date1_grater_than_date2(info->current_time, info->turn_off_time) == 1) \
                && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
                {
                    #ifdef DEBUG_MODULE
                        printf("PWM_AUTO_WORKING_PWM_OFF Toff mayor Ton \n");
                    #endif
                    info->output_status = PWM_OUTPUT_OFF;
                    pwm_manager_turn_off_pwm();
                    led_manager_send_pwm_info(0, 0, false);
                }
                else if((is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                    && (is_date1_grater_than_date2(info->turn_off_time, info->turn_on_time) == 1))
                {
                    info->output_status = PWM_OUTPUT_OFF;
                    pwm_manager_turn_off_pwm();
                    led_manager_send_pwm_info(0, 0, false);
                }
                else if((is_date1_grater_than_date2(info->current_time, info->turn_off_time) == 1) \
                    && (is_date1_grater_than_date2(info->turn_on_time, info->current_time) == 1) \
                    && (is_date1_grater_than_date2(info->turn_on_time, info->turn_off_time) == 1))
                {
                    #ifdef DEBUG_MODULE
                        printf("PWM_AUTO_WORKING_PWM_OFF Ton mayor Toff\n");
                    #endif
                    info->output_status = PWM_OUTPUT_OFF;
                    pwm_manager_turn_off_pwm();
                    led_manager_send_pwm_info(0, 0, false);
                    
                }
            }
        }
        pwm_percentage_ant = info->percent_power;
    }
}

//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------