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

#define QUEUE_ELEMENT_QUANTITY 20

#define MANUAL_PWM_TIME 3000000
#define MANUAL_TRIAC_TIME 3000000
#define RAMPA_PWM_TIME 500000
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

static esp_timer_handle_t timer_pwm;
static esp_timer_handle_t timer_triac;
static esp_timer_handle_t timer_pwm_rampa;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void config_led_power_up(void);
static void config_led_pwm_status(void);
static void config_led_rele_vege_status_up(void);
static void config_led_wifi_status(void);
static void config_led_triac_status(void);

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

static void timer_led_toggle_pwm_callback(void* arg);
static void timer_led_toggle_pwm_rampa_callback(void* arg);
static void timer_led_toggle_triac_callback(void* arg);

//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void timer_led_toggle_pwm_callback(void* arg)
{
    static int led_state_pwm = 0;
    gpio_set_level(PWM_OUTPUT_STATUS_LED, led_state_pwm);
    led_state_pwm = !led_state_pwm;
}
//------------------------------------------------------------------------------
static void timer_led_toggle_pwm_rampa_callback(void* arg)
{
    static int led_state_pwm_rampa = 0;
    gpio_set_level(PWM_OUTPUT_STATUS_LED, led_state_pwm_rampa);
    led_state_pwm_rampa = !led_state_pwm_rampa;
}
//------------------------------------------------------------------------------
static void timer_led_toggle_triac_callback(void* arg)
{
    static int led_state_triac = 0;
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, led_state_triac);
    led_state_triac = !led_state_triac;
}
//------------------------------------------------------------------------------
static void config_led_power_up(void)
{
    gpio_set_direction(DEVICE_ON_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(DEVICE_ON_LED, 0);
}
//------------------------------------------------------------------------------
static void config_led_pwm_status(void)
{
    gpio_set_direction(PWM_OUTPUT_STATUS_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PWM_OUTPUT_STATUS_LED, 0);

    esp_timer_create_args_t timer_pwm_args = {
        .callback = timer_led_toggle_pwm_callback,
        .arg = NULL,
        .name = "led_toggle_pwm"
    };

    esp_timer_create_args_t timer_pwm_rampa_args = {
        .callback = timer_led_toggle_pwm_rampa_callback,
        .arg = NULL,
        .name = "led_toggle_pwm_rampa"
    };

    esp_timer_create(&timer_pwm_args, &timer_pwm);
    esp_timer_create(&timer_pwm_rampa_args, &timer_pwm_rampa);
}
//------------------------------------------------------------------------------
static void config_led_rele_vege_status_up(void)
{
    gpio_set_direction(RELE_VEGE_STATUS_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(RELE_VEGE_STATUS_LED, 0);
}
//------------------------------------------------------------------------------
static void config_led_triac_status(void)
{
    gpio_set_direction(TRIAC_OUTPUT_STATUS_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, 0);

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
    gpio_set_direction(WIFI_STATUS_1_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(WIFI_STATUS_2_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(WIFI_STATUS_1_LED, 0);
    gpio_set_level(WIFI_STATUS_2_LED, 0);
}
//------------------------------------------------------------------------------
static void set_power_on_indicator(void)
{
    gpio_set_level(DEVICE_ON_LED, 1);
}
//------------------------------------------------------------------------------
static void set_pwm_auto_indicator(void)
{
    esp_timer_stop(timer_pwm_rampa);
    esp_timer_stop(timer_pwm);
    gpio_set_level(PWM_OUTPUT_STATUS_LED, 1);
}
//------------------------------------------------------------------------------
static void set_pwm_manual_off_indicator(void)
{
    esp_timer_stop(timer_pwm_rampa);
    esp_timer_stop(timer_pwm);
    gpio_set_level(PWM_OUTPUT_STATUS_LED, 0);
}
//------------------------------------------------------------------------------
static void set_triac_auto_indicator(void)
{
    esp_timer_stop(timer_triac);
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, 1);
}
//------------------------------------------------------------------------------
static void set_triac_manual_off_indicator(void)
{
    esp_timer_stop(timer_triac);
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, 0);
}
//------------------------------------------------------------------------------
static void set_rele_vege_on_indicator(void)
{
    printf("encender led");
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
    //led_manager_pwm_manual_on();
    //led_manager_pwm_rampa();
    //led_manager_triac_on();
    //led_manager_wifi_ap_mode();
    //led_manager_wifi_sta_mode();
    //led_manager_rele_vege_on();
    //set_wifi_net_problem_indicator();
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
                    esp_timer_stop(timer_pwm_rampa);
                    esp_timer_stop(timer_pwm);
                    set_pwm_manual_off_indicator();
                    break;
                case PWM_MANUAL_ON:
                    esp_timer_stop(timer_pwm_rampa);
                    esp_timer_start_periodic(timer_pwm, MANUAL_PWM_TIME);
                    break;
                case PWM_AUTO:
                    set_pwm_auto_indicator();
                    break;
                case PWM_RAMPA:
                    esp_timer_stop(timer_pwm_rampa);
                    esp_timer_start_periodic(timer_pwm_rampa, RAMPA_PWM_TIME);
                    break;
                case TRIAC_ON:
                    esp_timer_start_periodic(timer_triac, MANUAL_TRIAC_TIME);
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
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void led_manager_init(void)
{
    config_led_power_up();
    config_led_pwm_status();
    config_led_rele_vege_status_up();
    config_led_wifi_status();
    config_led_triac_status();

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
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------