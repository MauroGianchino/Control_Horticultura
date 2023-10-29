//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"
#include "freertos/queue.h"
#include "../include/pwm_manager.h"
#include "../include/board_def.h"
#include "../include/global_manager.h"
#include "driver/ledc.h"
#include "../include/led_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 15

#define PWM_TIMER LEDC_TIMER_0
#define PWM_MODE LEDC_HIGH_SPEED_MODE
#define PWM_OUTPUT_IO PWM_OUTPUT // Define the output GPIO
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define PWM_DUTY_50_PERCEN (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define PWM_FREQUENCY (1000) // Frequency in Hertz. Set frequency at 1 kHz

#define MIN_15_FADING_TIME 900000
#define SEC_1_FADING_TIME 1000

#define MAX_DUTY_CYCLE_STEPS 30
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    TURN_ON_PWM = 1,
    TURN_OFF_PWM = 2,
    UPDATE_DUTY_CYCLE = 3,
    TURN_ON_SIMUL_DAY_ON = 4,
    TURN_OFF_SIMUL_DAY_ON = 5,
    RESUME_FADING_FUNCTION = 6,
    ONLY_TURN_OFF_PWM = 7,
    GENERAL_UPDATE = 8,
}pwm_event_cmds_t;

typedef struct{
    pwm_event_cmds_t cmd;
    uint8_t duty_cycle;
}pwm_event_t;

typedef enum{
    FADING_IN_PROGRESS = 1,
    FADING_STOP = 2,
}fadng_status_t;

typedef enum{
    DUTY_CYCLE_FADING_ON = 0,
    DUTY_CYCLE_FADING_OFF = 1,
}fading_type_t;

typedef struct{
    fading_type_t fading_type;
    fadng_status_t fading_status;
    uint8_t duty_cycle;
    uint8_t out_duty_cycle;
    uint8_t step_duty_cycle;
    uint8_t step_number;
    uint8_t max_number_of_steps;
    uint32_t target_duty_cycle;
}manual_fading_info_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t pwm_manager_queue;

static fadng_status_t fading_status;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void pwm_manager_task(void* arg);
static void config_pwm_output(void);
static void turn_on_pwm(uint8_t duty_cycle);
static void update_pwm(uint8_t duty_cycle);
static void turn_off_pwm(void);
static void turn_on_pwm_simul_day_on(uint8_t duty_cycle);
static void turn_off_pwm_simul_day_on(void);
static void init_fading_on(uint8_t duty_cycle, manual_fading_info_t *manual_fading_info);
static void init_fading_off(uint8_t duty_cycle, manual_fading_info_t *manual_fading_info);
static void update_fading(manual_fading_info_t *manual_fading_info);
static void stop_fading(manual_fading_info_t *manual_fading_info);
void set_fading_status(fadng_status_t fading_status_aux);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
uint8_t is_fading_in_progress(void)
{
    if(fading_status == FADING_IN_PROGRESS)
    {
        return 1;
    }
    return 0;
}

void set_fading_status(fadng_status_t fading_status_aux)
{
    fading_status = fading_status_aux;
}

static void init_fading_on(uint8_t duty_cycle, manual_fading_info_t *manual_fading_info)
{
    if(manual_fading_info->duty_cycle < 30)
    {
        manual_fading_info->duty_cycle = 31;
    }
    manual_fading_info->fading_type = DUTY_CYCLE_FADING_ON;
    manual_fading_info->fading_status = FADING_IN_PROGRESS;
    manual_fading_info->duty_cycle = duty_cycle;
    manual_fading_info->out_duty_cycle = 10;
    manual_fading_info->step_number = 0;
    manual_fading_info->max_number_of_steps = MAX_DUTY_CYCLE_STEPS;
    manual_fading_info->step_duty_cycle = (manual_fading_info->duty_cycle - 10) / manual_fading_info->max_number_of_steps;
}
//------------------------------------------------------------------------------
static void init_fading_off(uint8_t duty_cycle, manual_fading_info_t *manual_fading_info)
{
    if(manual_fading_info->duty_cycle < 30)
    {
        manual_fading_info->duty_cycle = 31;
    }
    manual_fading_info->fading_type = DUTY_CYCLE_FADING_OFF;
    manual_fading_info->fading_status = FADING_IN_PROGRESS;
    manual_fading_info->duty_cycle = duty_cycle;
    manual_fading_info->out_duty_cycle = duty_cycle;
    manual_fading_info->step_number = 0;
    manual_fading_info->max_number_of_steps = MAX_DUTY_CYCLE_STEPS;
    manual_fading_info->step_duty_cycle = manual_fading_info->duty_cycle / manual_fading_info->max_number_of_steps;
}
//------------------------------------------------------------------------------
static void stop_fading(manual_fading_info_t *manual_fading_info)
{
    manual_fading_info->fading_status = FADING_STOP;
    manual_fading_info->step_number = 0;
}
//------------------------------------------------------------------------------
static void update_fading(manual_fading_info_t *manual_fading_info)
{
    uint32_t target_duty = 0;

    if(manual_fading_info->fading_type == DUTY_CYCLE_FADING_ON)
    {
        if(manual_fading_info->fading_status == FADING_IN_PROGRESS)
        {
            manual_fading_info->target_duty_cycle = (((uint32_t)manual_fading_info->out_duty_cycle)*(8191)) / 100;

            ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, manual_fading_info->target_duty_cycle, 10);
            ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);

            manual_fading_info->step_number++;
            manual_fading_info->out_duty_cycle += manual_fading_info->step_duty_cycle;

            if(manual_fading_info->out_duty_cycle > manual_fading_info->duty_cycle)
            {
                manual_fading_info->out_duty_cycle = manual_fading_info->duty_cycle;
            }

            if(manual_fading_info->step_number >= manual_fading_info->max_number_of_steps)
            {
                manual_fading_info->out_duty_cycle = manual_fading_info->duty_cycle;
                led_manager_send_pwm_info(manual_fading_info->out_duty_cycle, 0, false);
                manual_fading_info->target_duty_cycle = (((uint32_t)manual_fading_info->out_duty_cycle)*(8191)) / 100;

                ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, manual_fading_info->target_duty_cycle, 10);
                ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);

                manual_fading_info->fading_status = FADING_STOP;
            }
        }
    }
    else if(manual_fading_info->fading_type == DUTY_CYCLE_FADING_OFF)
    {
        if(manual_fading_info->fading_status == FADING_IN_PROGRESS)
        {
            manual_fading_info->target_duty_cycle = (((uint32_t)manual_fading_info->out_duty_cycle)*(8191)) / 100;

            ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, manual_fading_info->target_duty_cycle, 10);
            ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);

            manual_fading_info->step_number++;
            manual_fading_info->out_duty_cycle -= manual_fading_info->step_duty_cycle;

            if((manual_fading_info->step_number >= manual_fading_info->max_number_of_steps) \
                || (manual_fading_info->out_duty_cycle < 10)) 
            {
                manual_fading_info->out_duty_cycle = 0;
                led_manager_send_pwm_info(manual_fading_info->out_duty_cycle, 0, false);
                manual_fading_info->target_duty_cycle = (((uint32_t)manual_fading_info->out_duty_cycle)*(8191)) / 100;

                ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, manual_fading_info->target_duty_cycle, 10);
                ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
                manual_fading_info->fading_status = FADING_STOP;
            }
        }
    } 
}
//------------------------------------------------------------------------------
static void config_pwm_output(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = PWM_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution  = PWM_DUTY_RES,
        .freq_hz          = PWM_FREQUENCY,  // Set output frequency at 10 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = PWM_MODE,
        .channel        = PWM_CHANNEL,
        .timer_sel      = PWM_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PWM_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ledc_fade_func_install(0);
}
//------------------------------------------------------------------------------
static void turn_on_pwm(uint8_t duty_cycle)
{
    uint32_t target_duty = 0;

    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 41); // starts at duty cycle 1%
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);

    target_duty = (((uint32_t)duty_cycle)*(8191)) / 100;

    ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, target_duty, 10);
    ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
}
//------------------------------------------------------------------------------
static void turn_on_pwm_simul_day_on(uint8_t duty_cycle)
{
    uint32_t target_duty = 0;

    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 41); // starts at duty cycle 1%
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);

    target_duty = (((uint32_t)duty_cycle)*(8191)) / 100;

    ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, target_duty, MIN_15_FADING_TIME);
    ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
}
//------------------------------------------------------------------------------
static void turn_off_pwm_simul_day_on(void)
{
    ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, 0, MIN_15_FADING_TIME);
    ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
}
//------------------------------------------------------------------------------
static void update_pwm(uint8_t duty_cycle)
{
    uint32_t target_duty = 0;

    target_duty = (((uint32_t)duty_cycle)*(8191)) / 100;

    ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, target_duty, 10);
    ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
}
//------------------------------------------------------------------------------
static void turn_off_pwm(void)
{
    ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, 0, 10);
    ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
}
//------------------------------------------------------------------------------
static void pwm_manager_task(void* arg)
{
    pwm_event_t pwm_ev;
    manual_fading_info_t manual_fading_info;
    manual_fading_info.fading_status = FADING_STOP;
    uint32_t update_fading_periodicity = 1000;
    
    config_pwm_output();

    while(1)
    {
        if(xQueueReceive(pwm_manager_queue, &pwm_ev, update_fading_periodicity / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(pwm_ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case TURN_ON_PWM:
                    stop_fading(&manual_fading_info);
                    set_fading_status(FADING_STOP);
                    turn_on_pwm(pwm_ev.duty_cycle);
                    break; 
                case TURN_OFF_PWM:
                    stop_fading(&manual_fading_info);
                    set_fading_status(FADING_STOP);
                    turn_off_pwm();
                    break;
                case UPDATE_DUTY_CYCLE:
                    update_pwm(pwm_ev.duty_cycle);
                    break;
                case TURN_ON_SIMUL_DAY_ON:
                    //turn_on_pwm_simul_day_on(pwm_ev.duty_cycle);
                    init_fading_on(pwm_ev.duty_cycle, &manual_fading_info);
                    break;
                case TURN_OFF_SIMUL_DAY_ON:
                    //turn_off_pwm_simul_day_on();
                    init_fading_off(pwm_ev.duty_cycle, &manual_fading_info);
                    break;
                case RESUME_FADING_FUNCTION:
                    if(manual_fading_info.fading_status != FADING_STOP)
                    {
                        printf("RESUME FADING FUNCTION \n");
                        ledc_set_fade_with_time(PWM_MODE, PWM_CHANNEL, manual_fading_info.target_duty_cycle, 10);
                        ledc_fade_start(PWM_MODE, PWM_CHANNEL, LEDC_FADE_NO_WAIT);
                    }            
                    break;
                case GENERAL_UPDATE:
                    update_fading_periodicity = 1000;
                    break;
                case ONLY_TURN_OFF_PWM:
                    turn_off_pwm();
                    break;
            }       
        }
        else
        {
            update_fading(&manual_fading_info);
            set_fading_status(manual_fading_info.fading_status);

            if(manual_fading_info.fading_status == FADING_STOP)
            {
                update_fading_periodicity = 1000;
            }
            else if(manual_fading_info.fading_status == FADING_IN_PROGRESS)
            {
                update_fading_periodicity = 30000;
            }   
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void pwm_manager_init(void)
{
    pwm_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(pwm_event_t));

    xTaskCreate(pwm_manager_task, "pwm_manager_task", 
        configMINIMAL_STACK_SIZE*4, NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void pwm_manager_turn_on_pwm(uint8_t pwm_power_percent)
{
    pwm_event_t ev;

    ev.cmd = TURN_ON_PWM;
    ev.duty_cycle = pwm_power_percent;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_turn_off_pwm(void)
{
    pwm_event_t ev;

    ev.cmd = TURN_OFF_PWM;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_update_pwm(uint8_t pwm_power_percent)
{
    pwm_event_t ev;

    ev.cmd = UPDATE_DUTY_CYCLE;
    ev.duty_cycle = pwm_power_percent;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_turn_on_pwm_simul_day_on(uint8_t pwm_power_percent)
{
    pwm_event_t ev;

    ev.cmd = TURN_ON_SIMUL_DAY_ON;
    ev.duty_cycle = pwm_power_percent;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_turn_off_pwm_simul_day_on(uint8_t duty_cycle)
{
    pwm_event_t ev;

    ev.cmd = TURN_OFF_SIMUL_DAY_ON;
    ev.duty_cycle = duty_cycle;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_resume_fading_state_function(void)
{
    pwm_event_t ev;

    ev.cmd = RESUME_FADING_FUNCTION;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_only_turn_off_pwm(void)
{
    pwm_event_t ev;

    ev.cmd = ONLY_TURN_OFF_PWM;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void pwm_manager_general_update(void)
{
    pwm_event_t ev;

    ev.cmd = GENERAL_UPDATE;

    xQueueSend(pwm_manager_queue, &ev, 10);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------