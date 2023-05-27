//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "../include/led_manager.h"
#include "../include/board_def.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 20
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    DEVICE_POWER_ON = 1,
    PWM_MANUAL_OFF = 2,
    PWM_MANUAL_ON = 3,
    PWM_AUTO = 4,
    PWM_RAMPA = 5,
    TRIAC_ON = 6,
    TRIAC_OFF = 7,
    TRIAC_AUTO = 8,
    RELE_VEGE_ON = 9,
    RELE_VEGE_OFF = 10,
    WIFI_AP_MODE = 11,
    WIFI_STA_MODE = 12,
    WIFI_NET_PROBLEM = 13,
}led_event_cmds_t;

typedef struct{
    led_event_cmds_t cmd;
}led_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t led_manager_queue;
static led_event_t led_event;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void config_led_power_up(void);
static void config_led_pwm_status(void);
static void config_led_rele_vege_status_up(void);
static void config_led_wifi_status(void);

static void set_power_on_indicator(void);
static void set_pwm_auto_indicator(void);
static void set_pwm_manual_off_indicator(void);
static void set_triac_auto_indicator(void);
static void set_triac_manual_off_indicator(void);
static void set_rele_vege_on_indicator(void);
static void set_rele_vege_off_indicator(void);

static void set_wifi_ap_mode_indicator(void);
static void set_wifi_sta_mode_indicator(void);
static void set_wifi_net_problem_indicator(void);

static void led_manager_task(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void config_led_power_up(void)
{
    gpio_set_direction(DEVICE_ON_LED, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
static void config_led_pwm_status(void)
{
    gpio_set_direction(PWM_OUTPUT_STATUS_LED, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
static void config_led_rele_vege_status_up(void)
{
    gpio_set_direction(RELE_VEGE_STATUS_LED, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
static void config_led_wifi_status(void)
{
    gpio_set_direction(WIFI_STATUS_1_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(WIFI_STATUS_2_LED, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
static void set_power_on_indicator(void)
{
    gpio_set_level(DEVICE_ON_LED, 1);
}
//------------------------------------------------------------------------------
static void set_pwm_auto_indicator(void)
{
    gpio_set_level(PWM_OUTPUT_STATUS_LED, 1);
}
//------------------------------------------------------------------------------
static void set_pwm_manual_off_indicator(void)
{
    gpio_set_level(PWM_OUTPUT_STATUS_LED, 0);
}
//------------------------------------------------------------------------------
static void set_triac_auto_indicator(void)
{
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, 1);
}
//------------------------------------------------------------------------------
static void set_triac_manual_off_indicator(void)
{
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, 0);
}
//------------------------------------------------------------------------------
static void set_rele_vege_on_indicator(void)
{
    gpio_set_level(RELE_VEGE_STATUS_LED, 1);
}
//------------------------------------------------------------------------------
static void set_rele_vege_off_indicator(void)
{
    gpio_set_level(RELE_VEGE_STATUS_LED, 0);
}
//------------------------------------------------------------------------------
static void set_wifi_ap_mode_indicator(void)
{
    gpio_set_level(WIFI_STATUS_1_LED, 1);
    gpio_set_level(WIFI_STATUS_2_LED, 0);
}
//------------------------------------------------------------------------------
static void set_wifi_sta_mode_indicator(void)
{
    gpio_set_level(WIFI_STATUS_1_LED, 0);
    gpio_set_level(WIFI_STATUS_2_LED, 1);
}
//------------------------------------------------------------------------------
static void set_wifi_net_problem_indicator(void)
{
    gpio_set_level(WIFI_STATUS_1_LED, 1);
    gpio_set_level(WIFI_STATUS_2_LED, 1);
}
//------------------------------------------------------------------------------
static void led_manager_task(void* arg)
{
    //const char *LED_MANAGER_TASK_TAG = "LED_MANAGER_TASK_TAG";

    led_manager_power_up();

    while(1)
    {
        if(xQueueReceive(led_manager_queue, &led_event, portMAX_DELAY ) == pdTRUE)
        {
            switch(led_event.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                case DEVICE_POWER_ON:
                    set_power_on_indicator();
                    break;
                case PWM_MANUAL_OFF:
                    set_pwm_manual_off_indicator();
                    break;
                case PWM_MANUAL_ON:
                    break;
                case PWM_AUTO:
                    set_pwm_auto_indicator();
                    break;
                case PWM_RAMPA:
                    break;
                case TRIAC_ON:
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
                default:
                    break;
            }
        }
    }
}
//------------------------------------------------------------------------------
/*static void led_manager_task(void* arg)
{
    const char *LED_MANAGER_TASK_TAG = "LED_MANAGER_TASK_TAG";

    while(1)
    {
        gpio_set_level(DEVICE_ON_LED, 1);
#ifdef DEBUG_MODULE
        ESP_LOGI(LED_MANAGER_TASK_TAG, "set_led_pin_on");
#endif
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(DEVICE_ON_LED, 0);
#ifdef DEBUG_MODULE
        ESP_LOGI(LED_MANAGER_TASK_TAG, "set_led_pin_off");
#endif
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}*/
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void led_manager_init(void)
{
    config_led_power_up();
    config_led_pwm_status();
    config_led_rele_vege_status_up();
    config_led_wifi_status();

    led_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(led_event_t));
    
    xTaskCreate(led_manager_task, "led_manager_task", 2048, NULL, 10, NULL);
}
//------------------------------------------------------------------------------
void led_manager_power_up(void)
{
    led_event_t ev;

    ev.cmd = DEVICE_POWER_ON;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_pwm_manual_off(void)
{
    led_event_t ev;

    ev.cmd = PWM_MANUAL_OFF;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_pwm_manual_on(void)
{
    led_event_t ev;

    ev.cmd = PWM_MANUAL_ON;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_pwm_auto(void)
{
    led_event_t ev;

    ev.cmd = PWM_AUTO;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_pwm_rampa(void)
{
    led_event_t ev;

    ev.cmd = PWM_RAMPA;

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
void led_manager_pwm_triac_off(void)
{
    led_event_t ev;

    ev.cmd = TRIAC_OFF;

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
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------