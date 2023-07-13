//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "../include/board_def.h"
#include "../include/global_manager.h"
#include "../include/led_manager.h"
#include "../include/pwm_manager.h"

#include "../include/triac_manager.h"
#include "../include/triac_auto_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 200
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    PWM_MANUAL_ON = 1,
    PWM_OFF = 2,
    PWM_AUTO = 3,
    TRIAC_MANUAL_ON = 4,
    TRIAC_OFF = 5,
    TRIAC_AUTO = 6,
    RELE_VEGE_ON = 7,
    RELE_VEGE_OFF = 8,
    SET_MANUAL_PWM_POWER = 9,
    SET_AUTO_PWM_POWER = 10,
    UPDATE_CURRENT_TIME = 11,
    UPDATE_SIMUL_DAY_FUNCTION_STATUS = 12,
    UPDATE_PWM_CALENDAR = 13,
    UPDATE_TRIAC_CALENDAR = 14,
}global_event_cmds_t;

typedef struct{
    struct tm turn_on_time;
    struct tm turn_off_time;
    uint8_t enable;
}triac_info_t;

typedef struct{
    global_event_cmds_t cmd;
    uint8_t value;
    struct tm current_time;
    struct tm pwm_turn_on_time;
    struct tm pwm_turn_off_time;
    simul_day_status_t simul_day_function_status;
    triac_info_t triac_info[MAX_AUTO_TRIAC_CONFIGS_HOURS];
}global_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t global_manager_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void global_manager_task(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// TO DO: falta implementar funcion para actualizar parametros de hora del triac automatico
// UPDATE_TRIAC_CALENDAR
// TO DO: Falta agregar la dinamica de funcionamiento del rele vege
// TO DO: Falta agregar el guardado y el levantado de flash al bootear de los parametros
// no volatiles.
// Falta agregar una tarea que lleve el conteo de la hora y se lo actualice a la 
// global manager task.
static void global_manager_task(void* arg)
{
    global_event_t global_ev;
    nv_info_t global_info;
    triac_auto_info_t triac_auto_info;

    
    // PARA DEBUG HAY QUIE SUSTITUIR POR SECUENCIA DE STARTUP
    global_info.pwm_manual_percent_power = 10;
    global_manager_set_pwm_mode_off(); // EL PWM INICIA EN MANUAL APAGADO
    global_manager_set_triac_mode_off(); // EL TRIAC INICIA EN MANUAL APAGADO
    global_manager_update_simul_day_function_status(SIMUL_DAY_ON);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ////////////////////////////////////////////////////////
    while(1)
    {
        if(xQueueReceive(global_manager_queue, &global_ev, 2000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(global_ev.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                case UPDATE_CURRENT_TIME:
                    global_info.pwm_auto.current_time = mktime(&global_ev.current_time);
                    triac_auto_info.current_time = mktime(&global_ev.current_time);
                    break;
                case PWM_MANUAL_ON:
                    global_info.pwm_mode = MANUAL_ON;
                    led_manager_pwm_manual_on();
                    pwm_manager_turn_on_pwm(global_info.pwm_manual_percent_power);
                    pwm_auto_end();
                    break;
                case PWM_OFF:
                    global_info.pwm_mode = MANUAL_OFF;
                    led_manager_pwm_manual_off();
                    pwm_manager_turn_off_pwm();
                    pwm_auto_end();
                    break; 
                case PWM_AUTO:
                    global_info.pwm_mode = AUTOMATIC;
                    pwm_auto_start();
                    led_manager_pwm_auto();
                    break;
                case TRIAC_MANUAL_ON:
                    global_info.triac_mode = MANUAL_ON;
                    led_manager_triac_on();
                    triac_auto_end();
                    triac_manager_turn_on_triac();
                    break;
                case TRIAC_OFF:
                    global_info.triac_mode = MANUAL_OFF;
                    led_manager_triac_off();
                    triac_auto_end();
                    triac_manager_turn_off_triac();
                    break;
                case TRIAC_AUTO:
                    global_info.triac_mode = AUTOMATIC;
                    led_manager_triac_auto();
                    triac_auto_start();
                    break;
                case RELE_VEGE_ON:
                    global_info.rele_vege_status = RELE_ON;
                    led_manager_rele_vege_on();
                    break;
                case RELE_VEGE_OFF:
                    global_info.rele_vege_status = RELE_OFF;
                    led_manager_rele_vege_off();
                    break;
                case SET_MANUAL_PWM_POWER:
                    global_info.pwm_manual_percent_power = global_ev.value;
                    if(global_info.pwm_mode == MANUAL_ON)
                    {
                        pwm_manager_update_pwm(global_info.pwm_manual_percent_power);
                        #ifdef DEBUG_MODULE
                            printf("UPDATE PWM: %d \n", global_info.pwm_manual_percent_power);
                        #endif
                    }
                    break;
                case SET_AUTO_PWM_POWER:
                    global_info.pwm_auto.percent_power = global_ev.value;
                    if(global_info.pwm_mode == AUTOMATIC)
                    {
                        pwm_manager_update_pwm(global_info.pwm_auto.percent_power);
                        #ifdef DEBUG_MODULE
                            printf("UPDATE PWM: %d \n", global_info.pwm_auto.percent_power);
                        #endif
                    }
                    break;
                case UPDATE_SIMUL_DAY_FUNCTION_STATUS:
                    global_info.pwm_auto.simul_day_status = global_ev.simul_day_function_status;
                    break;
                case UPDATE_PWM_CALENDAR:
                    global_info.pwm_auto.turn_on_time = mktime(&global_ev.pwm_turn_on_time); 
                    global_info.pwm_auto.turn_off_time = mktime(&global_ev.pwm_turn_off_time);
                    break;
                case UPDATE_TRIAC_CALENDAR:
                    triac_auto_info.triac_auto[0].enable = global_ev.triac_info[0].enable;
                    triac_auto_info.triac_auto[1].enable = global_ev.triac_info[1].enable;
                    triac_auto_info.triac_auto[2].enable = global_ev.triac_info[2].enable;
                    triac_auto_info.triac_auto[3].enable = global_ev.triac_info[3].enable;
                    triac_auto_info.triac_auto[0].turn_off_time = mktime(&global_ev.triac_info[0].turn_off_time);
                    triac_auto_info.triac_auto[1].turn_off_time = mktime(&global_ev.triac_info[1].turn_off_time);
                    triac_auto_info.triac_auto[2].turn_off_time = mktime(&global_ev.triac_info[2].turn_off_time);
                    triac_auto_info.triac_auto[3].turn_off_time = mktime(&global_ev.triac_info[3].turn_off_time);
                    triac_auto_info.triac_auto[0].turn_on_time = mktime(&global_ev.triac_info[0].turn_on_time);
                    triac_auto_info.triac_auto[1].turn_on_time = mktime(&global_ev.triac_info[1].turn_on_time);
                    triac_auto_info.triac_auto[2].turn_on_time = mktime(&global_ev.triac_info[2].turn_on_time);
                    triac_auto_info.triac_auto[3].turn_on_time = mktime(&global_ev.triac_info[3].turn_on_time);
                    break;
                default:
                    break;
            }
        }
        else
        {
            pwm_auto_manager_handler(&global_info.pwm_auto);
            triac_auto_manager_handler(&triac_auto_info);
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void global_manager_init(void)
{
    global_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(global_event_t));
    
    xTaskCreate(global_manager_task, "global_manager_task", configMINIMAL_STACK_SIZE*4, 
        NULL, configMAX_PRIORITIES-1, NULL);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_mode_off(void)
{
    global_event_t ev;
    ev.cmd = PWM_OFF;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_mode_manual_on(void)
{
    global_event_t ev;
    ev.cmd = PWM_MANUAL_ON;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_mode_auto(void)
{
    global_event_t ev;
    ev.cmd = PWM_AUTO;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_triac_mode_off(void)
{
    global_event_t ev;
    ev.cmd = TRIAC_OFF;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_triac_manual_on(void)
{
    global_event_t ev;
    ev.cmd = TRIAC_MANUAL_ON;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_triac_mode_auto(void)
{
    global_event_t ev;
    ev.cmd = TRIAC_AUTO;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_rele_vege_status_off(void)
{
    global_event_t ev;
    ev.cmd = RELE_VEGE_OFF;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_rele_vege_status_on(void)
{
    global_event_t ev;
    ev.cmd = RELE_VEGE_ON;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_power_value_manual(uint8_t power_percentage_value)
{
    global_event_t ev;
    if(power_percentage_value >= 98)
        power_percentage_value = 98;
    ev.cmd = SET_MANUAL_PWM_POWER;
    ev.value = power_percentage_value;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_update_current_time(struct tm current_time)
{
    global_event_t ev;
    ev.cmd = UPDATE_CURRENT_TIME;
    ev.current_time = current_time;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_update_simul_day_function_status(simul_day_status_t status)
{
    global_event_t ev;
    ev.cmd = UPDATE_SIMUL_DAY_FUNCTION_STATUS;
    ev.simul_day_function_status = status;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_update_auto_pwm_calendar(calendar_auto_mode_t calendar)
{
    global_event_t ev;
    ev.cmd = UPDATE_PWM_CALENDAR;
    ev.pwm_turn_on_time = calendar.turn_on_time;
    ev.pwm_turn_off_time = calendar.turn_off_time;
    xQueueSend(global_manager_queue, &ev, 10);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------
