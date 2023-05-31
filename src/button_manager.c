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

#define TIEMPO_ANTIRREBOTE_MS 50
#define TIEMPO_PULSADO_MS 3000

//#define DEBUG_MODULE
//------------------------------TYPEDEF-----------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED,
    WIFI_BUTTON_PUSHED,
    WIFI_BUTTON_PUSHED_3_SEC,
}cmds_t;

typedef struct{
    cmds_t cmd;
}button_events_t;
//--------------------DECLARACION DE DATOS INTERNOS-----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t button_manager_queue;
static button_events_t button_events;

static TickType_t push_init_time_wifi_button;
//--------------------DECLARACION DE FUNCIONES INTERNAS-------------------------
//------------------------------------------------------------------------------
static void config_wifi_mode_button(void);
static void button_event_manager_task(void * pvParameters);
static void wifi_mode_button_interrupt(void *arg);
//--------------------DEFINICION DE DATOS INTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE DATOS EXTERNOS------------------------------
//------------------------------------------------------------------------------

//--------------------DEFINICION DE FUNCIONES INTERNAS--------------------------
//------------------------------------------------------------------------------
static void config_wifi_mode_button(void)
{
    gpio_config_t config;

    // Configurar el pin GPIO34 como entrada
    config.pin_bit_mask = (1ULL << WIFI_MODE_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&config);

    // Configurar la interrupción del botón
    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIFI_MODE_BUTTON, wifi_mode_button_interrupt, NULL);

    push_init_time_wifi_button = 0;
}
//------------------------------------------------------------------------------
static void IRAM_ATTR wifi_mode_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if(current_time - push_init_time_wifi_button >= pdMS_TO_TICKS(TIEMPO_PULSADO_MS)) 
    {
        // Se ha mantenido pulsado durante más de 3 segundos
        ev.cmd = WIFI_BUTTON_PUSHED_3_SEC;
    }
    else if((current_time - push_init_time_wifi_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
    {
        // Se ha mantenido pulsado menos de 3 segundos 
        // Paso correctamente el antirrebote
        ev.cmd = WIFI_BUTTON_PUSHED;
    }
    push_init_time_wifi_button = current_time;
    xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
}
//------------------------------------------------------------------------------
void button_event_manager_task(void * pvParameters)
{
    config_wifi_mode_button();
    
    while(true)
    {
        if(xQueueReceive(button_manager_queue, &button_events, portMAX_DELAY) == pdTRUE)
        {
            switch(button_events.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case WIFI_BUTTON_PUSHED:
                    //gpio_set_level(GPIO_NUM_4, 1);
                    //gpio_set_level(GPIO_NUM_5, 0);
                    break;
                case WIFI_BUTTON_PUSHED_3_SEC:
                    //gpio_set_level(GPIO_NUM_4, 0);
                    //gpio_set_level(GPIO_NUM_5, 1);
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
