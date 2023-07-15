//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>
#include <string.h>

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
#include "../include/nv_flash_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 200
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    SET_DEVICE_ALIAS = 1,
    SET_PWM_MODE = 2,
    PWM_MANUAL_ON = 3,
    PWM_OFF = 4,
    PWM_AUTO = 5,
    TRIAC_MANUAL_ON = 6,
    TRIAC_OFF = 7,
    TRIAC_AUTO = 8,
    RELE_VEGE_ON = 9,
    RELE_VEGE_OFF = 10,
    SET_MANUAL_PWM_POWER = 11,
    SET_AUTO_PWM_POWER = 12,
    UPDATE_CURRENT_TIME = 13,
    UPDATE_SIMUL_DAY_FUNCTION_STATUS = 14,
    UPDATE_PWM_CALENDAR = 15,
    UPDATE_TRIAC_CALENDAR = 16,
}global_event_cmds_t;

typedef struct{
    struct tm turn_on_time;
    struct tm turn_off_time;
    uint8_t enable;
}triac_info_t;

typedef struct{
    global_event_cmds_t cmd;
    output_mode_t output_mode;
    uint8_t value;
    char str_value[80];  
    struct tm current_time;
    struct tm pwm_turn_on_time;
    struct tm pwm_turn_off_time;
    simul_day_status_t simul_day_function_status;
    triac_info_t triac_info[MAX_AUTO_TRIAC_CONFIGS_HOURS];
    bool value_read_from_flash;
}global_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t global_manager_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void global_manager_task(void* arg);
static void nv_init_alias(void);
static void nv_save_alias(char *alias);
static void nv_init_pwm_mode(void);
static void nv_save_pwm_mode(output_mode_t pwm_mode);
static void nv_init_simul_day_status(void);
void nv_save_simul_day_status(simul_day_status_t simul_day_status);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void nv_init_alias(void)
{
    char alias[DEVICE_ALIAS_MAX_LENGTH];
    memset(alias, '\0', sizeof(alias));

    if(read_str_from_flash(DEVICE_ALIAS_KEY, alias))
    {
        #ifdef DEBUG_MODULE
            printf("ALIAS READ: %s \n", alias);
        #endif
        global_manager_set_device_alias(alias, true);
    }
    else
    {
        #ifdef DEBUG_MODULE
            printf("PWM MODE READING FAILED \n");
        #endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_alias(char *alias)
{
    write_parameter_on_flash_str(DEVICE_ALIAS_KEY, alias); 
}
//------------------------------------------------------------------------------
static void nv_init_pwm_mode(void)
{
    uint32_t value;
    output_mode_t pwm_mode;
    if(read_uint32_from_flash(PWM_MODE_KEY, &value))
    {
        pwm_mode = (output_mode_t)value;
        #ifdef DEBUG_MODULE
            printf("PWM MODE READ: %d \n", pwm_mode);
        #endif
        global_manager_set_pwm_mode(pwm_mode);
    }
    else
    {
        #ifdef DEBUG_MODULE
            printf("PWM MODE READING FAILED \n");
        #endif
    }
}
//------------------------------------------------------------------------------
void nv_save_pwm_mode(output_mode_t pwm_mode)
{
    write_parameter_on_flash_uint32(PWM_MODE_KEY, (uint32_t)pwm_mode);
}
//------------------------------------------------------------------------------
static void nv_init_simul_day_status(void)
{
    uint32_t value;
    simul_day_status_t simul_day_status;
    if(read_uint32_from_flash(SIMUL_DAY_STATUS_KEY, &value))
    {
        simul_day_status = (simul_day_status_t)value;
        #ifdef DEBUG_MODULE
            printf("SIMUL DAY STATUS READ: %d \n", simul_day_status);
        #endif
        global_manager_update_simul_day_function_status(simul_day_status, true);
    }
    else
    {
        #ifdef DEBUG_MODULE
            printf("SIMUL DAY STATUS READING FAILED \n");
        #endif
    }
}
//------------------------------------------------------------------------------
void nv_save_simul_day_status(simul_day_status_t simul_day_status)
{
    write_parameter_on_flash_uint32(SIMUL_DAY_STATUS_KEY, (uint32_t)simul_day_status);
}
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

    global_info.pwm_manual_percent_power = 10;

    // INIT FROM FLASH
    nv_init_alias();
    nv_init_pwm_mode();
    nv_init_simul_day_status();
    // PARA DEBUG HAY QUIE SUSTITUIR POR SECUENCIA DE STARTUP
    
    global_manager_set_triac_mode_off(); // EL TRIAC INICIA EN MANUAL APAGADO
    //global_manager_update_simul_day_function_status(SIMUL_DAY_ON);
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
                case SET_DEVICE_ALIAS:
                    if((strcmp((const char*)global_info.device_alias, (const char*)global_ev.str_value) != 0) \
                        && (global_ev.value_read_from_flash == false))
                    {
                        nv_save_alias(global_ev.str_value);
                    }
                    strcpy(global_info.device_alias, global_ev.str_value);
                    break;
                case SET_PWM_MODE:
                    global_info.pwm_mode = global_ev.output_mode;
                    switch(global_info.pwm_mode)
                    {
                        case MANUAL_ON:
                            led_manager_pwm_manual_on();
                            pwm_manager_turn_on_pwm(global_info.pwm_manual_percent_power);
                            pwm_auto_end();
                            break;
                        case MANUAL_OFF:
                            led_manager_pwm_manual_off();
                            pwm_manager_turn_off_pwm();
                            pwm_auto_end();
                            break;
                        case AUTOMATIC:
                            pwm_auto_start();
                            led_manager_pwm_auto();
                            break;
                        default:
                            break;
                    }
                    break;
                case PWM_MANUAL_ON:
                    if(global_info.pwm_mode != MANUAL_ON)
                    {
                        nv_save_pwm_mode(MANUAL_ON);
                    }
                    global_info.pwm_mode = MANUAL_ON;
                    led_manager_pwm_manual_on();
                    pwm_manager_turn_on_pwm(global_info.pwm_manual_percent_power);
                    pwm_auto_end();
                    break;
                case PWM_OFF:
                    if(global_info.pwm_mode != MANUAL_OFF)
                    {
                        nv_save_pwm_mode(MANUAL_OFF);
                    }
                    global_info.pwm_mode = MANUAL_OFF;
                    led_manager_pwm_manual_off();
                    pwm_manager_turn_off_pwm();
                    pwm_auto_end();
                    break; 
                case PWM_AUTO:
                    if(global_info.pwm_mode != AUTOMATIC)
                    {
                        nv_save_pwm_mode(AUTOMATIC);
                    }
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
                    
                    if((global_info.pwm_mode == MANUAL_ON) && (global_ev.value != global_info.pwm_manual_percent_power))
                    {
                        pwm_manager_update_pwm(global_ev.value);
                        #ifdef DEBUG_MODULE
                            printf("UPDATE PWM: %d \n", global_info.pwm_manual_percent_power);
                        #endif
                    }
                    global_info.pwm_manual_percent_power = global_ev.value;
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
                    if((global_info.pwm_auto.simul_day_status != global_ev.simul_day_function_status)\
                        && (global_ev.value_read_from_flash == false))
                    {
                        nv_save_simul_day_status(global_ev.simul_day_function_status);
                    }
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
    
    xTaskCreate(global_manager_task, "global_manager_task", configMINIMAL_STACK_SIZE*8, 
        NULL, configMAX_PRIORITIES-1, NULL);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_mode(output_mode_t pwm_mode)
{
    global_event_t ev;
    ev.cmd = SET_PWM_MODE;
    ev.output_mode = pwm_mode;
    xQueueSend(global_manager_queue, &ev, 10);
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
void global_manager_update_simul_day_function_status(simul_day_status_t status , bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = UPDATE_SIMUL_DAY_FUNCTION_STATUS;
    ev.value_read_from_flash = read_from_flash;
    ev.simul_day_function_status = status;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
uint8_t global_manager_set_device_alias(char* alias, bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = SET_DEVICE_ALIAS;
  
    strncpy(ev.str_value, alias, strlen(alias));
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
    return 1;
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
