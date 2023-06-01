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

#include "../include/button_manager.h"
#include "../include/board_def.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define QUEUE_ELEMENT_QUANTITY 20

#define TIEMPO_ANTIRREBOTE_MS 40
#define TIEMPO_PULSADO_MS 3000

//#define DEBUG_MODULE
//------------------------------TYPEDEF-----------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED,
    WIFI_BUTTON_PUSHED,
    WIFI_BUTTON_PUSHED_3_SEC,
    PWM_BUTTON_PUSHED,
    PWM_BUTTON_PUSHED_3_SEC,
    TRIAC_BUTTON_PUSHED,
    TRIAC_BUTTON_PUSHED_3_SEC,
    VEGE_BUTTON_PUSHED,
    VEGE_BUTTON_PUSHED_3_SEC,
    SIMUL_POTE_POS_BUTTON_PUSHED,
    SIMUL_POTE_NEG_BUTTON_PUSHED,
}cmds_t;

typedef struct{
    cmds_t cmd;
}button_events_t;
//--------------------DECLARACION DE DATOS INTERNOS-----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t button_manager_queue;
static button_events_t button_events;

static TickType_t push_init_time_wifi_button;
static TickType_t push_init_time_pwm_button;
static TickType_t push_init_time_triac_button;
static TickType_t push_init_time_vege_button;
static TickType_t push_init_time_simul_pote_pos_button;
static TickType_t push_init_time_simul_pote_neg_button;
//--------------------DECLARACION DE FUNCIONES INTERNAS-------------------------
//------------------------------------------------------------------------------
static void button_event_manager_task(void * pvParameters);

static void config_buttons_isr(void);

static void wifi_mode_button_interrupt(void *arg);
static void pwm_button_interrupt(void *arg);
static void triac_button_interrupt(void *arg);
static void vege_button_interrupt(void *arg);
static void simul_pote_pos_button_interrupt(void *arg);
static void simul_pote_neg_button_interrupt(void *arg);
//--------------------DEFINICION DE DATOS INTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE DATOS EXTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE FUNCIONES INTERNAS--------------------------
//------------------------------------------------------------------------------
static void config_buttons_isr(void)
{
    gpio_config_t config;

    // Configurar el pin GPIO34 como entrada
    config.pin_bit_mask = (1ULL << WIFI_MODE_BUTTON) | (1ULL << PWM_BUTTON) | (1ULL << TRIAC_BUTTON) | (1ULL << VEGE_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&config);

    config.pin_bit_mask = (1ULL << SIMUL_POTE_POS_BUTTON) | (1ULL << SIMUL_POTE_NEG_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&config);

    // Configurar la interrupción del botón
    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIFI_MODE_BUTTON, wifi_mode_button_interrupt, NULL);
    gpio_isr_handler_add(PWM_BUTTON, pwm_button_interrupt, NULL);
    gpio_isr_handler_add(TRIAC_BUTTON, triac_button_interrupt, NULL);
    gpio_isr_handler_add(VEGE_BUTTON, vege_button_interrupt, NULL);
    gpio_isr_handler_add(SIMUL_POTE_POS_BUTTON, simul_pote_pos_button_interrupt, NULL);
    gpio_isr_handler_add(SIMUL_POTE_NEG_BUTTON, simul_pote_neg_button_interrupt, NULL);

    push_init_time_wifi_button = 0;
    push_init_time_pwm_button = 0;
    push_init_time_triac_button =0;
    push_init_time_vege_button = 0;
    push_init_time_simul_pote_pos_button = 0;
    push_init_time_simul_pote_neg_button = 0;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR wifi_mode_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if(current_time - push_init_time_wifi_button >= pdMS_TO_TICKS(TIEMPO_PULSADO_MS)) 
    {
        ev.cmd = WIFI_BUTTON_PUSHED_3_SEC;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    else if((current_time - push_init_time_wifi_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        ev.cmd = WIFI_BUTTON_PUSHED;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    push_init_time_wifi_button = current_time;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR pwm_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if(current_time - push_init_time_pwm_button >= pdMS_TO_TICKS(TIEMPO_PULSADO_MS)) 
    {
        ev.cmd = PWM_BUTTON_PUSHED_3_SEC;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    else if((current_time - push_init_time_pwm_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        ev.cmd = PWM_BUTTON_PUSHED;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    push_init_time_pwm_button = current_time;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR triac_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if(current_time - push_init_time_triac_button >= pdMS_TO_TICKS(TIEMPO_PULSADO_MS)) 
    {
        ev.cmd = TRIAC_BUTTON_PUSHED_3_SEC;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    else if((current_time - push_init_time_triac_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        ev.cmd = TRIAC_BUTTON_PUSHED;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    push_init_time_triac_button = current_time;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR vege_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if(current_time - push_init_time_vege_button >= pdMS_TO_TICKS(TIEMPO_PULSADO_MS)) 
    {
        ev.cmd = VEGE_BUTTON_PUSHED_3_SEC;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    else if((current_time - push_init_time_vege_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        ev.cmd = VEGE_BUTTON_PUSHED;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    push_init_time_vege_button = current_time;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR simul_pote_pos_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if((current_time - push_init_time_simul_pote_pos_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        ev.cmd = SIMUL_POTE_POS_BUTTON_PUSHED;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    push_init_time_simul_pote_pos_button = current_time;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR simul_pote_neg_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if((current_time - push_init_time_simul_pote_neg_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        ev.cmd = SIMUL_POTE_NEG_BUTTON_PUSHED;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
    push_init_time_simul_pote_neg_button = current_time;  
}
//------------------------------------------------------------------------------
void button_event_manager_task(void * pvParameters)
{
    //bool state = 0;
    config_buttons_isr();

    while(true)
    {
        if(xQueueReceive(button_manager_queue, &button_events, portMAX_DELAY) == pdTRUE)
        {
            switch(button_events.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case WIFI_BUTTON_PUSHED:
                    gpio_set_level(GPIO_NUM_4, 1);
                    gpio_set_level(GPIO_NUM_5, 0);
                    break;
                case WIFI_BUTTON_PUSHED_3_SEC:
                    gpio_set_level(GPIO_NUM_4, 0);
                    gpio_set_level(GPIO_NUM_5, 1);
                    break;
                case PWM_BUTTON_PUSHED:
                    //gpio_set_level(GPIO_NUM_4, 1);
                    //gpio_set_level(GPIO_NUM_5, 0);
                    break;
                case PWM_BUTTON_PUSHED_3_SEC:
                    //gpio_set_level(GPIO_NUM_4, 0);
                    //gpio_set_level(GPIO_NUM_5, 1);
                    break;
                case TRIAC_BUTTON_PUSHED:
                    //gpio_set_level(GPIO_NUM_4, 1);
                    //gpio_set_level(GPIO_NUM_5, 0);
                    break;
                case TRIAC_BUTTON_PUSHED_3_SEC:
                    //gpio_set_level(GPIO_NUM_4, 0);
                    //gpio_set_level(GPIO_NUM_5, 1);
                    break;
                case VEGE_BUTTON_PUSHED:
                    //gpio_set_level(GPIO_NUM_4, 1);
                    //gpio_set_level(GPIO_NUM_5, 0);
                    break;
                case VEGE_BUTTON_PUSHED_3_SEC:
                    //gpio_set_level(GPIO_NUM_4, 0);
                    //gpio_set_level(GPIO_NUM_5, 1);
                    break;
                case SIMUL_POTE_POS_BUTTON_PUSHED:
                    //gpio_set_level(GPIO_NUM_4, state);
                    //state = !state;
                    break;
                case SIMUL_POTE_NEG_BUTTON_PUSHED:
                    
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
               configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL);             
}
//--------------------FIN DEL ARCHIVO-------------------------------------------
//------------------------------------------------------------------------------
