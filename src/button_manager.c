// NOTAS: No se puede printear ni loguear desde una ISR se reinicia el ESP32.
//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "esp_timer.h"

#include "../include/button_manager.h"
#include "../include/board_def.h"
#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define QUEUE_ELEMENT_QUANTITY 20

#define TIEMPO_ANTIRREBOTE_MS 50
#define TIEMPO_PULSADO_MS 3000

//#define DEBUG_MODULE
//TO DO: ver el tema del button wifi y los led wifi
//------------------------------TYPEDEF-----------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED,
    PWM_BUTTON_PUSHED,
    PWM_BUTTON_PUSHED_3_SEC,
    TRIAC_BUTTON_PUSHED,
    TRIAC_BUTTON_PUSHED_3_SEC,
    VEGE_BUTTON_PUSHED,
}cmds_t;

typedef struct{
    cmds_t cmd;
}button_events_t;
//--------------------DECLARACION DE DATOS INTERNOS-----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t button_manager_queue;

volatile int64_t start_time = 0;
volatile int64_t start_time_triac = 0;
volatile int64_t start_time_vege = 0;
//--------------------DECLARACION DE FUNCIONES INTERNAS-------------------------
//------------------------------------------------------------------------------
static void button_event_manager_task(void * pvParameters);

static void config_buttons_isr(void);

static void pwm_button_interrupt(void *arg);
static void triac_button_interrupt(void *arg);
static void vege_button_interrupt(void *arg);
//--------------------DEFINICION DE DATOS INTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE DATOS EXTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE FUNCIONES INTERNAS--------------------------
//------------------------------------------------------------------------------
static void config_buttons_isr(void)
{
    gpio_config_t config;

    config.pin_bit_mask = (1ULL << VEGE_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLDOWN_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&config);
    
    config.pin_bit_mask = (1ULL << PWM_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLDOWN_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&config);

    config.pin_bit_mask = (1ULL << TRIAC_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLDOWN_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&config);

    // Configurar la interrupción del botón
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PWM_BUTTON, pwm_button_interrupt, NULL);
    gpio_isr_handler_add(TRIAC_BUTTON, triac_button_interrupt, NULL);
    gpio_isr_handler_add(VEGE_BUTTON, vege_button_interrupt, NULL);
}
//------------------------------------------------------------------------------
static void IRAM_ATTR pwm_button_interrupt(void *arg) 
{
    button_events_t ev;
    int64_t time_now = esp_timer_get_time();

    if(gpio_get_level(PWM_BUTTON) == 0)
    {
        start_time = time_now;
    }
    else if(gpio_get_level(PWM_BUTTON) == 1)
    {
        if (start_time != 0)
        {
            int64_t diff = time_now - start_time;
            if (diff > 3000000)  // 3 seconds expressed in microseconds
            { 
                ev.cmd = PWM_BUTTON_PUSHED_3_SEC;
            }
            else if (diff > 30000)  // 30ms seconds expressed in microseconds
            {
                 ev.cmd = PWM_BUTTON_PUSHED;
            }
            start_time = 0;
            xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
        }
    }
}
//------------------------------------------------------------------------------
static void IRAM_ATTR triac_button_interrupt(void *arg) 
{
    button_events_t ev;
    int64_t time_now = esp_timer_get_time();

    if(gpio_get_level(TRIAC_BUTTON) == 0)
    {
        start_time_triac = time_now;
    }
    else if(gpio_get_level(TRIAC_BUTTON) == 1)
    {
        if (start_time_triac != 0)
        {
            int64_t diff = time_now - start_time_triac;
            if (diff > 3000000)  // 3 seconds expressed in microseconds
            { 
                ev.cmd = TRIAC_BUTTON_PUSHED_3_SEC;
            }
            else if (diff > 30000)  // 30ms seconds expressed in microseconds
            {
                 ev.cmd = TRIAC_BUTTON_PUSHED;
            }
            start_time_triac = 0;
            xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
        }
    }
}
//------------------------------------------------------------------------------
static void IRAM_ATTR vege_button_interrupt(void *arg) 
{
    button_events_t ev;
    int64_t time_now = esp_timer_get_time();

    if(gpio_get_level(VEGE_BUTTON) == 0)
    {
        start_time_vege = time_now;
    }
    else if(gpio_get_level(VEGE_BUTTON) == 1)
    {
        if (start_time_vege != 0)
        {
            int64_t diff = time_now - start_time_vege;

            if (diff > 30000)  // 30ms seconds expressed in microseconds
            {
                 ev.cmd = VEGE_BUTTON_PUSHED;
            }
            start_time_vege = 0;
            xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
        }
    }
}
//------------------------------------------------------------------------------
void button_event_manager_task(void * pvParameters)
{
    button_events_t button_ev;
    
    output_mode_t pwm_status = MANUAL_OFF;
    output_mode_t triac_status = MANUAL_OFF;
    rele_output_status_t rele_vege_status = RELE_VEGE_DISABLE;
#ifdef DIGITAL_POTE
    uint8_t pwm_percent_power = 0;
#endif

    config_buttons_isr();

    while(true)
    {
        if(xQueueReceive(button_manager_queue, &button_ev, portMAX_DELAY) == pdTRUE)
        {
            switch(button_ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case PWM_BUTTON_PUSHED:
                    if(pwm_status == MANUAL_OFF)
                    {
                        global_manager_set_pwm_mode_off();
                        pwm_status = AUTOMATIC;
                    }
                    else if(pwm_status == AUTOMATIC)
                    {
                        global_manager_set_pwm_mode_auto();
                        pwm_status = MANUAL_OFF;
                    }
                    break;
                case PWM_BUTTON_PUSHED_3_SEC:
                    global_manager_set_pwm_mode_manual_on();
                    pwm_status = MANUAL_OFF;
                    break;
                case TRIAC_BUTTON_PUSHED:
                    if(triac_status == MANUAL_OFF)
                    {
                        global_manager_set_triac_mode_off(false);
                        triac_status = AUTOMATIC;
                    }
                    else if(triac_status == AUTOMATIC)
                    {
                        global_manager_set_triac_mode_auto(false);
                        triac_status = MANUAL_OFF;
                    }
                    break;
                case TRIAC_BUTTON_PUSHED_3_SEC:
                    global_manager_set_triac_mode_manual_on(false);
                    triac_status = MANUAL_OFF;
                    break;
                case VEGE_BUTTON_PUSHED:
                    if(rele_vege_status == RELE_VEGE_DISABLE)
                    {
                        global_manager_set_rele_vege_status_off(false);
                        rele_vege_status = RELE_VEGE_ENABLE;
                    }
                    else if(rele_vege_status == RELE_VEGE_ENABLE)
                    {
                        global_manager_set_rele_vege_status_on(false);
                        rele_vege_status = RELE_VEGE_DISABLE;
                    }
                    break;
                default:
                break;
            }
        }
    }
}
//--------------------DEFINICION DE FUNCIONES EXTERNAS--------------------------
//------------------------------------------------------------------------------
void button_manager_init(void)
{
    button_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(button_events_t));

    xTaskCreate(button_event_manager_task, "button_event_manager_task", 
               configMINIMAL_STACK_SIZE*5, NULL, configMAX_PRIORITIES, NULL);             
}
//--------------------FIN DEL ARCHIVO-------------------------------------------
//------------------------------------------------------------------------------
