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
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 20
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
}global_event_cmds_t;

typedef struct{
    global_event_cmds_t cmd;
    uint8_t value;
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
static void global_manager_task(void* arg)
{
    global_event_t global_ev;
    nv_info_t global_info;
    
    // PARA DEBUG HAY QUIE SUSTITUIR POR SECUENCIA DE STARTUP
    global_manager_set_pwm_mode_manual_on(); // EL PWM INICIA EN MANUAL
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ////////////////////////////////////////////////////////
    while(1)
    {
        if(xQueueReceive(global_manager_queue, &global_ev, portMAX_DELAY ) == pdTRUE)
        {
            switch(global_ev.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                case PWM_MANUAL_ON:
                    global_info.pwm_mode = MANUAL_ON;
                    led_manager_pwm_manual_on();
                    pwm_manager_turn_on_pwm(global_info.pwm_percent_power);
                    break;
                case PWM_OFF:
                    global_info.pwm_mode = MANUAL_OFF;
                    led_manager_pwm_manual_off();
                    pwm_manager_turn_off_pwm();
                    break; 
                case PWM_AUTO:
                    global_info.pwm_mode = AUTOMATIC;
                    led_manager_pwm_auto();
                    break;
                case TRIAC_MANUAL_ON:
                    global_info.triac_mode = MANUAL_ON;
                    led_manager_triac_on();
                    break;
                case TRIAC_OFF:
                    global_info.triac_mode = MANUAL_OFF;
                    led_manager_triac_off();
                    break;
                case TRIAC_AUTO:
                    global_info.triac_mode = AUTOMATIC;
                    led_manager_triac_auto();
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
                    if(global_info.pwm_mode == MANUAL_ON)
                    {
                        global_info.pwm_percent_power = global_ev.value;
                        pwm_manager_update_pwm(global_info.pwm_percent_power);
                        #ifdef DEBUG_MODULE
                            printf("PORCENTAJE POTENCIA PWM: %d \n", global_info.pwm_percent_power);
                        #endif
                    }
                    break;
                case SET_AUTO_PWM_POWER:
                    if(global_info.pwm_mode == AUTOMATIC)
                    {
                        global_info.pwm_percent_power = global_ev.value;
                        pwm_manager_update_pwm(global_info.pwm_percent_power);
                        #ifdef DEBUG_MODULE
                            printf("PORCENTAJE POTENCIA PWM: %d \n", global_info.pwm_percent_power);
                        #endif
                    }
                    break;
                default:
                    break;
            }
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
    assert(power_percentage_value <= MAX_PERCENTAGE_POWER_VALUE);
    ev.cmd = SET_MANUAL_PWM_POWER;
    ev.value = power_percentage_value;
    xQueueSend(global_manager_queue, &ev, 10);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------