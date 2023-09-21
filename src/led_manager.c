//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "../include/led_manager.h"
#include "../include/board_def.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 10

#define MANUAL_DEVICE_MODE_TIME 200000
#define MANUAL_TRIAC_TIME 200000
#define RAMPA_PWM_TIME 800000

#define LED_ON 0
#define LED_OFF 1
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    DEVICE_POWER_ON = 1,
    TRIAC_ON = 2,
    TRIAC_OFF = 3,
    TRIAC_AUTO = 4,
    RELE_VEGE_ON = 5,
    RELE_VEGE_OFF = 6,
    WIFI_AP_MODE = 7,
    WIFI_STA_MODE = 8,
    WIFI_NET_PROBLEM = 9,
    DEVICE_MODE_AUTO = 10,
    DEVICE_MODE_MANUAL = 11,
}led_event_cmds_t;

typedef struct{
    led_event_cmds_t cmd;
}led_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
QueueHandle_t led_manager_queue;

static esp_timer_handle_t timer_device_mode;
static esp_timer_handle_t timer_triac;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void config_led_power_up(void);
static void config_led_device_mode_status(void);
static void config_led_rele_vege_status_up(void);
static void config_led_wifi_status(void);
static void config_led_triac_status(void);

static void set_power_on_indicator(void);
static void set_triac_auto_indicator(void);
static void set_triac_manual_off_indicator(void);
static void set_rele_vege_on_indicator(void);
static void set_rele_vege_off_indicator(void);

static void set_wifi_ap_mode_indicator(void);
static void set_wifi_sta_mode_indicator(void);
static void set_wifi_net_problem_indicator(void);

static void led_manager_task(void* arg);

static void timer_led_toggle_device_mode_callback(void* arg);
static void timer_led_toggle_triac_callback(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void timer_led_toggle_device_mode_callback(void* arg)
{
    static int led_state_pwm = LED_OFF;
    gpio_set_level(DEVICE_MODE_LED, led_state_pwm);
    led_state_pwm = !led_state_pwm;
}
//------------------------------------------------------------------------------
static void timer_led_toggle_triac_callback(void* arg)
{
    static int led_state_triac = LED_OFF;
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, led_state_triac);
    led_state_triac = !led_state_triac;
}
//------------------------------------------------------------------------------
static void config_led_power_up(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << DEVICE_ON_LED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(DEVICE_ON_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void config_led_device_mode_status(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << DEVICE_MODE_LED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(DEVICE_MODE_LED, LED_OFF);

    esp_timer_create_args_t timer_device_mode_args = {
        .callback = timer_led_toggle_device_mode_callback,
        .arg = NULL,
        .name = "led_toggle_device_mode"
    };

    esp_timer_create(&timer_device_mode_args, &timer_device_mode);
}
//------------------------------------------------------------------------------
static void config_led_rele_vege_status_up(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << RELE_VEGE_STATUS_LED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(RELE_VEGE_STATUS_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void config_led_triac_status(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << TRIAC_OUTPUT_STATUS_LED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, LED_OFF);

    esp_timer_create_args_t timer_triac_args = {
        .callback = timer_led_toggle_triac_callback,
        .arg = NULL,
        .name = "led_toggle_triac"
    };

    esp_timer_create(&timer_triac_args, &timer_triac);
}
//------------------------------------------------------------------------------
static void config_led_wifi_status(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << WIFI_STATUS_1_LED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << WIFI_STATUS_2_LED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(WIFI_STATUS_1_LED, LED_OFF);
    gpio_set_level(WIFI_STATUS_2_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void set_power_on_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("POWER ON \n");
    #endif
    gpio_set_level(DEVICE_ON_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_device_mode_auto_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("DEVICE MODE AUTO \n");
    #endif
    esp_timer_stop(timer_device_mode);
    gpio_set_level(DEVICE_MODE_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_triac_auto_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("TRIAC AUTO \n");
    #endif
    esp_timer_stop(timer_triac);
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_triac_manual_off_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("TRIAC OFF \n");
    #endif
    esp_timer_stop(timer_triac);
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void set_rele_vege_on_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("Led rele vege ON \n");
    #endif
    gpio_set_level(RELE_VEGE_STATUS_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_rele_vege_off_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("Led rele vege OFF \n");
    #endif
    gpio_set_level(RELE_VEGE_STATUS_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void set_wifi_ap_mode_indicator(void)
{
    gpio_set_level(WIFI_STATUS_1_LED, LED_ON);
    gpio_set_level(WIFI_STATUS_2_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void set_wifi_sta_mode_indicator(void)
{
    gpio_set_level(WIFI_STATUS_1_LED, LED_OFF);
    gpio_set_level(WIFI_STATUS_2_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_wifi_net_problem_indicator(void)
{
    gpio_set_level(WIFI_STATUS_1_LED, LED_ON);
    gpio_set_level(WIFI_STATUS_2_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void led_manager_task(void* arg)
{
    //const char *LED_MANAGER_TASK_TAG = "LED_MANAGER_TASK_TAG";
    led_event_t led_ev;

    led_manager_power_up();

    while(1)
    {
        if(xQueueReceive(led_manager_queue, &led_ev, portMAX_DELAY ) == pdTRUE)
        {
            switch(led_ev.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                case DEVICE_POWER_ON:
                    set_power_on_indicator();
                    break;
                case TRIAC_ON:
                    esp_timer_start_periodic(timer_triac, MANUAL_TRIAC_TIME);
                    #ifdef DEBUG_MODULE
                        printf("TRIAC MANUAL ON \n");
                    #endif
                    break;
                case TRIAC_OFF:
                    set_triac_manual_off_indicator();
                    break;
                case TRIAC_AUTO:
                    set_triac_auto_indicator();
                    break;
                case RELE_VEGE_ON:
                    set_rele_vege_on_indicator();
                    break;
                case RELE_VEGE_OFF:
                    set_rele_vege_off_indicator();
                    break;
                case WIFI_AP_MODE:
                    set_wifi_ap_mode_indicator();
                    break;
                case WIFI_STA_MODE:
                    set_wifi_sta_mode_indicator();
                    break;
                case WIFI_NET_PROBLEM:
                    set_wifi_net_problem_indicator();
                    break;
                case DEVICE_MODE_AUTO:
                    set_device_mode_auto_indicator();
                    break;
                case DEVICE_MODE_MANUAL:
                    esp_timer_start_periodic(timer_device_mode, MANUAL_DEVICE_MODE_TIME);
                    break;
                default:
                    break;
            }
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void led_manager_init(void)
{
    config_led_power_up();
    config_led_device_mode_status();
    config_led_rele_vege_status_up();
    config_led_wifi_status();
    config_led_triac_status();

    led_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(led_event_t));
    
    xTaskCreate(led_manager_task, "led_manager_task", configMINIMAL_STACK_SIZE*2, 
        NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void led_manager_power_up(void)
{
    led_event_t ev;

    ev.cmd = DEVICE_POWER_ON;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_triac_on(void)
{
    led_event_t ev;

    ev.cmd = TRIAC_ON;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_triac_auto(void)
{
    led_event_t ev;

    ev.cmd = TRIAC_AUTO;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_triac_off(void)
{
    led_event_t ev;

    ev.cmd = TRIAC_OFF;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_rele_vege_on(void)
{
    led_event_t ev;

    ev.cmd = RELE_VEGE_ON;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_rele_vege_off(void)
{
    led_event_t ev;

    ev.cmd = RELE_VEGE_OFF;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_wifi_ap_mode(void)
{
    led_event_t ev;

    ev.cmd = WIFI_AP_MODE;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_wifi_sta_mode(void)
{
    led_event_t ev;

    ev.cmd = WIFI_STA_MODE;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_wifi_net_problem(void)
{
    led_event_t ev;

    ev.cmd = WIFI_NET_PROBLEM;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_set_device_mode_auto(void)
{
    led_event_t ev;

    ev.cmd = DEVICE_MODE_AUTO;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_set_device_mode_manual(void)
{
    led_event_t ev;

    ev.cmd = DEVICE_MODE_MANUAL;

    xQueueSend(led_manager_queue, &ev, 10);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------