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
#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define QUEUE_ELEMENT_QUANTITY 20

#define TIEMPO_ANTIRREBOTE_MS 40
#define TIEMPO_PULSADO_MS 3000

//#define DEBUG_MODULE
//TO DO: ver el tema del button wifi y los led wifi
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
    SIMUL_POTE_POS_BUTTON_PUSHED,
    SIMUL_POTE_NEG_BUTTON_PUSHED,
}cmds_t;

typedef struct{
    cmds_t cmd;
}button_events_t;
//--------------------DECLARACION DE DATOS INTERNOS-----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t button_manager_queue;

static TickType_t push_init_time_vege_button;
static TickType_t push_init_time_simul_pote_pos_button;
static TickType_t push_init_time_simul_pote_neg_button;

volatile int triac_button_pressed_time = 0;
volatile int pwm_button_pressed_time = 0;
volatile int wifi_button_pressed_time = 0;
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
    config.pin_bit_mask = (1ULL << VEGE_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&config);

    config.pin_bit_mask = (1ULL << WIFI_MODE_BUTTON) | (1ULL << PWM_BUTTON) | (1ULL << TRIAC_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&config);

    config.pin_bit_mask = (1ULL << SIMUL_POTE_POS_BUTTON) | (1ULL << SIMUL_POTE_NEG_BUTTON);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&config);

    // Configurar la interrupción del botón
    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIFI_MODE_BUTTON, wifi_mode_button_interrupt, NULL);
    gpio_isr_handler_add(PWM_BUTTON, pwm_button_interrupt, NULL);
    gpio_isr_handler_add(TRIAC_BUTTON, triac_button_interrupt, NULL);
    gpio_isr_handler_add(VEGE_BUTTON, vege_button_interrupt, NULL);
    gpio_isr_handler_add(SIMUL_POTE_POS_BUTTON, simul_pote_pos_button_interrupt, NULL);
    gpio_isr_handler_add(SIMUL_POTE_NEG_BUTTON, simul_pote_neg_button_interrupt, NULL);

    push_init_time_vege_button = xTaskGetTickCount();
    push_init_time_simul_pote_pos_button = xTaskGetTickCount();
    push_init_time_simul_pote_neg_button = xTaskGetTickCount();
}
//------------------------------------------------------------------------------
static void IRAM_ATTR wifi_mode_button_interrupt(void *arg) 
{
    button_events_t ev;
    uint32_t currentTime = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS; 

    if(gpio_get_level(WIFI_MODE_BUTTON) == 0)
    {
        wifi_button_pressed_time += currentTime;
    }
    else
    {
        if(currentTime - wifi_button_pressed_time >= TIEMPO_PULSADO_MS)
        {
            ev.cmd = WIFI_BUTTON_PUSHED_3_SEC;
        }
        else if(currentTime - wifi_button_pressed_time >= TIEMPO_ANTIRREBOTE_MS)
        {
            ev.cmd = WIFI_BUTTON_PUSHED;
        }
        wifi_button_pressed_time = 0;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
}
//------------------------------------------------------------------------------
static void IRAM_ATTR pwm_button_interrupt(void *arg) 
{
    button_events_t ev;
    uint32_t currentTime = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS; 

    if(gpio_get_level(PWM_BUTTON) == 0)
    {
        pwm_button_pressed_time += currentTime;
    }
    else
    {
        if(currentTime - pwm_button_pressed_time >= TIEMPO_PULSADO_MS)
        {
            ev.cmd = PWM_BUTTON_PUSHED_3_SEC;
        }
        else if(currentTime - pwm_button_pressed_time >= TIEMPO_ANTIRREBOTE_MS)
        {
            ev.cmd = PWM_BUTTON_PUSHED;
        }
        pwm_button_pressed_time = 0;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
}
//------------------------------------------------------------------------------
static void IRAM_ATTR triac_button_interrupt(void *arg) 
{
    button_events_t ev;
    uint32_t currentTime = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS; // Tiempo actual en milisegundos

    // Verifica si el botón está presionado
    if(gpio_get_level(TRIAC_BUTTON) == 0)
    {
        // Si el botón está presionado, incrementa el contador de tiempo
        triac_button_pressed_time += currentTime;
    }
    else
    {
        // Si el botón se ha soltado, verifica si se ha mantenido presionado durante más de 3 segundos
        if(currentTime - triac_button_pressed_time >= TIEMPO_PULSADO_MS)
        {
            ev.cmd = TRIAC_BUTTON_PUSHED_3_SEC;
        }
        else if(currentTime - triac_button_pressed_time >= TIEMPO_ANTIRREBOTE_MS)
        {
            ev.cmd = TRIAC_BUTTON_PUSHED;
        }
        triac_button_pressed_time = 0;
        xQueueSendFromISR(button_manager_queue, &ev, pdFALSE);
    }
}
//------------------------------------------------------------------------------
static void IRAM_ATTR vege_button_interrupt(void *arg) 
{
    TickType_t current_time = xTaskGetTickCountFromISR();
    button_events_t ev;

    if((current_time - push_init_time_vege_button >= pdMS_TO_TICKS(TIEMPO_ANTIRREBOTE_MS)))
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
                case WIFI_BUTTON_PUSHED:
                    gpio_set_level(GPIO_NUM_4, 1);
                    gpio_set_level(GPIO_NUM_5, 0);
                    printf("WIFI BUTTON PUSHED\n");
                    break;
                case WIFI_BUTTON_PUSHED_3_SEC:
                    gpio_set_level(GPIO_NUM_4, 0);
                    gpio_set_level(GPIO_NUM_5, 1);
                    printf("WIFI BUTTON PUSHED 3 sec\n");
                    break;
                case PWM_BUTTON_PUSHED:
                    if(pwm_status == MANUAL_OFF)
                    {
                        //printf("PWM BUTTON PUSHED MANUAL OFF\n");
                        global_manager_set_pwm_mode_off();
                        pwm_status = AUTOMATIC;
                    }
                    else if(pwm_status == AUTOMATIC)
                    {
                        //printf("PWM BUTTON PUSHED MANUAL AUTOMATIC\n");
                        global_manager_set_pwm_mode_auto();
                        pwm_status = MANUAL_OFF;
                    }
                    break;
                case PWM_BUTTON_PUSHED_3_SEC:
                    //printf("PWM BUTTON PUSHED 3 SECONDS \n");
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
                case SIMUL_POTE_POS_BUTTON_PUSHED:
                    #ifdef DIGITAL_POTE
                        if(pwm_percent_power < MAX_PERCENTAGE_POWER_VALUE - 5)
                        {
                            pwm_percent_power += 5;
                            global_manager_set_pwm_power_value_manual(pwm_percent_power);
                        }                
                    #endif
                    break;
                case SIMUL_POTE_NEG_BUTTON_PUSHED:
                    #ifdef DIGITAL_POTE
                        if(pwm_percent_power > MIN_PERCENTAGE_POWER_VALUE + 5)
                        {
                            pwm_percent_power -= 5;
                            global_manager_set_pwm_power_value_manual(pwm_percent_power);
                        }   
                    #endif            
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
