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

#define QUEUE_ELEMENT_QUANTITY 25

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
    RELE_VEGE_ON = 5,
    RELE_VEGE_OFF = 6,
    DEVICE_MODE_AUTO = 7,
    DEVICE_MODE_MANUAL = 8,
    UPDATE_PWM_LED_STATUS = 9,
}led_event_cmds_t;

typedef struct{
    led_event_cmds_t cmd;
    uint8_t duty_cycle;
    simul_day_status_t simul_day_status;
    uint8_t is_simul_day_working;
}led_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
QueueHandle_t led_manager_queue;
static esp_timer_handle_t timer_led_power_new_update;

static bool led_power_new_update = false;
static esp_timer_handle_t timer_pwm_status;
static uint8_t pwm_toggle_mode = 0;
static bool restart_timer = true;  
static bool led_power_started = false;

//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void config_led_power_up(void);
static void config_led_device_mode_status(void);
static void config_led_rele_vege_status_up(void);
static void config_led_pwm_status(void);
static void config_led_triac_status(void);

static void set_power_on_indicator(void);
static void set_triac_output_on_indicator(void);
static void set_triac_output_off_indicator(void);
static void set_rele_vege_on_indicator(void);
static void set_rele_vege_off_indicator(void);
static void set_pwm_indicator(uint8_t duty_cycle, uint8_t is_simul_day_working);

static void set_device_mode_manual_indicator(void);

static void led_manager_task(void* arg);

static void timer_led_toggle_pwm_status_callback(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void timer_led_toggle_device_mode_callback(void* arg)
{
    static int led_state_device_mode = LED_OFF;
    gpio_set_level(DEVICE_MODE_LED, led_state_device_mode);
    led_state_device_mode = !led_state_device_mode;
}

static int pwm_led_red_status = LED_OFF;
static int pwm_led_green_status = LED_OFF;
//------------------------------------------------------------------------------
static void timer_led_power_new_update_callback(void* arg)
{
    led_power_new_update = false;
    led_power_started = false;
}
//------------------------------------------------------------------------------
static void timer_led_toggle_pwm_status_callback(void* arg)
{
    if(pwm_toggle_mode == 1)
    {
        gpio_set_level(PWM_LED_RED, pwm_led_red_status);
        gpio_set_level(PWM_LED_GREEN, pwm_led_green_status);
        pwm_led_green_status = !pwm_led_green_status;
    }
    else if(pwm_toggle_mode == 2)
    {
        gpio_set_level(PWM_LED_RED, pwm_led_red_status);
        gpio_set_level(PWM_LED_GREEN, pwm_led_green_status);
        pwm_led_red_status = !pwm_led_red_status;
        pwm_led_green_status = !pwm_led_green_status;
    }
    else if(pwm_toggle_mode == 3)
    {
        gpio_set_level(PWM_LED_RED, pwm_led_red_status);
        gpio_set_level(PWM_LED_GREEN, pwm_led_green_status);
        pwm_led_red_status = !pwm_led_red_status;
    }
    else if(pwm_toggle_mode == 4)
    {
        gpio_set_level(PWM_LED_RED, pwm_led_red_status);
        gpio_set_level(PWM_LED_GREEN, pwm_led_green_status);

        if((pwm_led_green_status == LED_OFF) && (pwm_led_red_status == LED_ON))
        {
            pwm_led_red_status = LED_OFF;
            pwm_led_green_status = LED_ON;
        }
        else if((pwm_led_green_status == LED_ON) && (pwm_led_red_status == LED_OFF))
        {
            pwm_led_red_status = LED_ON;
            pwm_led_green_status = LED_ON;
        }
        else if((pwm_led_green_status == LED_ON) && (pwm_led_red_status == LED_ON))
        {
            pwm_led_red_status = LED_ON;
            pwm_led_green_status = LED_OFF;
        }
    }
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
}
//------------------------------------------------------------------------------
static void config_led_pwm_status(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << PWM_LED_RED); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = (1ULL << PWM_LED_GREEN); // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(PWM_LED_RED, LED_OFF);
    gpio_set_level(PWM_LED_GREEN, LED_OFF);

    esp_timer_create_args_t timer_pwm_status_args = {
        .callback = timer_led_toggle_pwm_status_callback,
        .arg = NULL,
        .name = "led_toggle_timer_pwm_status"
    };

    esp_timer_create(&timer_pwm_status_args, &timer_pwm_status);
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
    gpio_set_level(DEVICE_MODE_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_device_mode_manual_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("DEVICE MODE MANUAL \n");
    #endif
    gpio_set_level(DEVICE_MODE_LED, LED_OFF);
}
//------------------------------------------------------------------------------
static void set_triac_output_on_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("TRIAC ON \n");
    #endif
    gpio_set_level(TRIAC_OUTPUT_STATUS_LED, LED_ON);
}
//------------------------------------------------------------------------------
static void set_triac_output_off_indicator(void)
{
    #ifdef DEBUG_MODULE
        printf("TRIAC OFF \n");
    #endif
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
static void set_pwm_indicator(uint8_t duty_cycle, uint8_t is_simul_day_working)
{
    float pwm_time;

    pwm_time = 1.78 * (duty_cycle) + 22;

    //printf(" tiempo de toggle de pwm %f ms \n", pwm_time);
    //printf(" duty cycle de pwm %d \n", duty_cycle);

    if(duty_cycle < 10)
    {
        esp_timer_stop(timer_pwm_status);
        restart_timer = true;
        gpio_set_level(PWM_LED_RED, LED_OFF);
        gpio_set_level(PWM_LED_GREEN, LED_OFF);
    }
    else
    {
        if(is_simul_day_working != 1)
        {
            if((duty_cycle > 10) && (duty_cycle < 34))
            {
                pwm_toggle_mode = 1;
            }
            else if((duty_cycle >= 34) && (duty_cycle < 67))
            {
                pwm_toggle_mode = 2;
            }
            else if((duty_cycle >= 67) && (duty_cycle < 100))
            {
                pwm_toggle_mode = 3;
            }
            //
            if(restart_timer == true)
            {
                restart_timer = false;
                esp_timer_stop(timer_pwm_status);
                esp_timer_start_periodic(timer_pwm_status, 75000);
            }
            
            pwm_led_red_status = LED_OFF;
            pwm_led_green_status = LED_OFF;
        }
        else
        {
            restart_timer = true;
            esp_timer_stop(timer_pwm_status);
            pwm_toggle_mode = 4;
            esp_timer_start_periodic(timer_pwm_status, 500000);
            pwm_led_red_status = LED_ON;
            pwm_led_green_status = LED_OFF;
        }
    }
}
//------------------------------------------------------------------------------
static void led_power_task(void* args)
{
    
    esp_timer_create_args_t timer_led_power_new_update_args = {
        .callback = timer_led_power_new_update_callback,
        .arg = NULL,
        .name = "timer_led_power_new_update"
    };

    esp_timer_create(&timer_led_power_new_update_args, &timer_led_power_new_update);

    while(1)
    {
        if(led_power_new_update == false)
        {
            gpio_set_level(DEVICE_ON_LED, LED_ON);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            gpio_set_level(DEVICE_ON_LED, LED_OFF);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        else if(led_power_new_update == true)
        {
            if(led_power_started == false)
            {
                led_power_started = true;
                esp_timer_stop(timer_led_power_new_update);
                esp_timer_start_once(timer_led_power_new_update, 500000);
            }
            gpio_set_level(DEVICE_ON_LED, LED_ON);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            gpio_set_level(DEVICE_ON_LED, LED_OFF);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}
//------------------------------------------------------------------------------
static void led_manager_task(void* arg)
{
    //const char *LED_MANAGER_TASK_TAG = "LED_MANAGER_TASK_TAG";
    led_event_t led_ev;
    uint8_t pwm_duty_cycle;
    simul_day_status_t simul_day_status;
    uint8_t is_simul_day_working;

    //led_manager_power_up();

    while(1)
    {
        if(xQueueReceive(led_manager_queue, &led_ev, portMAX_DELAY ) == pdTRUE)
        {
            switch(led_ev.cmd)
            {
                case  CMD_UNDEFINED:
                    break;
                case DEVICE_POWER_ON:
                    //set_power_on_indicator();
                    break;
                case TRIAC_ON:
                    set_triac_output_on_indicator();
                    break;
                case TRIAC_OFF:
                    set_triac_output_off_indicator();
                    break;
                case RELE_VEGE_ON:
                    set_rele_vege_on_indicator();
                    break;
                case RELE_VEGE_OFF:
                    set_rele_vege_off_indicator();
                    break;
                case DEVICE_MODE_AUTO:
                    set_device_mode_auto_indicator();
                    break;
                case DEVICE_MODE_MANUAL:
                    set_device_mode_manual_indicator();
                    break;
                case UPDATE_PWM_LED_STATUS:
                    pwm_duty_cycle = led_ev.duty_cycle;
                    simul_day_status = led_ev.simul_day_status;
                    is_simul_day_working = led_ev.is_simul_day_working;
                    set_pwm_indicator(pwm_duty_cycle, is_simul_day_working);
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
    config_led_pwm_status();
    config_led_triac_status();

    led_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(led_event_t));
    
    xTaskCreate(led_manager_task, "led_manager_task", configMINIMAL_STACK_SIZE*10, 
        NULL, configMAX_PRIORITIES-2, NULL);

    xTaskCreate(led_power_task, "led_power_task", configMINIMAL_STACK_SIZE*4, 
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
//------------------------------------------------------------------------------
void led_manager_send_pwm_info(uint8_t duty_cycle, uint8_t is_simul_day_working, simul_day_status_t simul_day_status)
{
    led_event_t ev;

    ev.cmd = UPDATE_PWM_LED_STATUS;
    ev.duty_cycle = duty_cycle;
    ev.is_simul_day_working = is_simul_day_working;
    ev.simul_day_status = simul_day_status;

    xQueueSend(led_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void led_manager_new_update(void)
{
    led_power_new_update = true;
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------