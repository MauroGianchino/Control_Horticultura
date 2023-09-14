//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

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
#include "../include/triac_manager.h"
#include "../include/vege_manager.h"
#include "../include/nv_flash_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 50

#define TIMEOUT_MS 500
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum
{
    CMD_UNDEFINED = 0,
    SET_PWM_MODE = 1,
    PWM_MANUAL_ON = 2,
    PWM_OFF = 3,
    PWM_AUTO = 4,
    TRIAC_MANUAL_ON = 5,
    TRIAC_OFF = 6,
    TRIAC_AUTO = 7,
    RELE_VEGE_ON = 8,
    RELE_VEGE_OFF = 9,
    SET_MANUAL_PWM_POWER = 10,
    SET_AUTO_PWM_POWER = 11,
    UPDATE_CURRENT_TIME = 12,
    UPDATE_SIMUL_DAY_FUNCTION_STATUS = 13,
    UPDATE_PWM_CALENDAR = 14,
    UPDATE_TRIAC_CALENDAR = 15,
    SET_SSID = 16,
    SET_PASSWORD = 17,
    GET_CONFIG_NET_INFO = 18,
    GET_CONFIG_PWM_INFO = 19,
    GET_CONFIG_TRIAC_INFO = 20,
    GET_CONFIG_RELE_VEGE_INFO = 21,
} global_event_cmds_t;

typedef struct
{
    global_event_cmds_t cmd;
    output_mode_t output_mode;
    uint8_t value;
    char str_value[80];
    struct tm current_time;
    struct tm pwm_turn_on_time;
    struct tm pwm_turn_off_time;
    simul_day_status_t simul_day_function_status;
    triac_config_info_t triac_info;
    uint8_t triac_num;
    bool value_read_from_flash;
} global_event_t;

typedef struct
{
    char ssid[33];
    char password[40];
    output_mode_t pwm_mode;
    pwm_auto_info_t pwm_auto;
    output_mode_t triac_mode;
    triac_auto_info_t triac_auto;
    rele_output_status_t rele_vege_status;
} response_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t global_manager_queue, response_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void global_manager_task(void *arg);
static void nv_init_triac_mode(void);
static void nv_save_triac_mode(output_mode_t triac_mode);
static void nv_init_pwm_mode(void);
static void nv_save_pwm_mode(output_mode_t pwm_mode);
static void nv_init_simul_day_status(void);
static void nv_save_simul_day_status(simul_day_status_t simul_day_status);
static void nv_save_rele_vege_status(rele_output_status_t rele_vege_status);
static void nv_init_auto_percent_power(void);
static void nv_save_auto_percent_power(uint8_t percent_power);
static void nv_init_pwm_calendar(void);
static void nv_save_pwm_calendar(pwm_auto_info_t pwm_calendar);
static void nv_init_triac_calendar(uint8_t triac_num);
static void nv_save_triac_calendar(triac_config_info_t triac_info, uint8_t triac_num);
static void nv_init_ssid_ap_wifi(void);
static void nv_save_ssid_ap_wifi(char *ssid);
static void nv_init_password_ap_wifi(void);
static void nv_save_password_ap_wifi(char *password);

static void get_net_info(void);
static uint8_t wait_net_info_response(char *ssid, char *password);
static void get_pwm_info(void);
static uint8_t wait_pwm_info_response(output_mode_t *pwm_mode, pwm_auto_info_t *pwm_auto);
static void get_triac_info(void);
static uint8_t wait_triac_info_response(output_mode_t *triac_mode, triac_auto_info_t *triac_auto);
static void get_rele_vege_info(void);
static uint8_t wait_rele_vege_info_response(rele_output_status_t *rele_vege_status);

static uint8_t check_later_hour(struct tm hour_1, struct tm hour_2);
static uint8_t check_30_min_difference_between_hours(struct tm hour_1, struct tm hour_2);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
// Si devuelve 1 hour_1 es mas tarde que hour_2
static uint8_t check_later_hour(struct tm hour_1, struct tm hour_2)
{
    if (hour_1.tm_hour > hour_2.tm_hour)
    {
        return 1;
    }
    else if ((hour_1.tm_hour == hour_2.tm_hour) && ((hour_1.tm_min > hour_2.tm_min)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
//------------------------------------------------------------------------------
// Si devuelve 1 hay una diferencia mayor a 30 minutos entre hour_1 y hour_2 y hour_1 es mayor que hour_2
static uint8_t check_30_min_difference_between_hours(struct tm hour_1, struct tm hour_2)
{
    if ((hour_1.tm_hour == hour_2.tm_hour) && ((hour_1.tm_min - 30 > hour_2.tm_min)))
    {
        return 1;
    }
    else if ((hour_1.tm_hour > hour_2.tm_hour) && ((hour_1.tm_min + 60 - hour_2.tm_min) > 30))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
//------------------------------------------------------------------------------
static void nv_init_ssid_ap_wifi(void)
{
    char ssid[DEVICE_SSID_MAX_LENGTH];
    memset(ssid, '\0', sizeof(ssid));

    if (read_str_from_flash(WIFI_AP_SSID_KEY, ssid))
    {
#ifdef DEBUG_MODULE
        printf("SSID READ: %s \n", ssid);
#endif
        global_manager_set_wifi_ssid(ssid, true);
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("SSID READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_ssid_ap_wifi(char *ssid)
{
    write_parameter_on_flash_str(WIFI_AP_SSID_KEY, ssid);
}
//------------------------------------------------------------------------------
static void nv_init_password_ap_wifi(void)
{
    char password[DEVICE_PASS_MAX_LENGTH];
    memset(password, '\0', sizeof(password));

    if (read_str_from_flash(WIFI_AP_PASSWORD_KEY, password))
    {
#ifdef DEBUG_MODULE
        printf("WIFI PASSWORD READ: %s \n", password);
#endif
        global_manager_set_wifi_password(password, true);
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("WIFI PASSWORD READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_password_ap_wifi(char *password)
{
    write_parameter_on_flash_str(WIFI_AP_PASSWORD_KEY, password);
}
//------------------------------------------------------------------------------
static void nv_init_triac_calendar(uint8_t triac_num)
{
    triac_config_info_t info;
    char aux_str_on[20], aux_str_off[20], triac_enable_aux[20];
    uint32_t value;

    switch (triac_num)
    {
    case 1:
        strcpy(aux_str_on, TRIAC1_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC1_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC1_DATE_ENABLE);
        break;
    case 2:
        strcpy(aux_str_on, TRIAC2_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC2_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC2_DATE_ENABLE);
        break;
    case 3:
        strcpy(aux_str_on, TRIAC3_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC3_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC3_DATE_ENABLE);
        break;
    case 4:
        strcpy(aux_str_on, TRIAC4_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC4_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC4_DATE_ENABLE);
        break;
    default:
        break;
    }

    if (read_date_from_flash(aux_str_on, &info.turn_on_time))
    {
#ifdef DEBUG_MODULE
        printf("TURN ON TIME HOUR READ: %d \n", info.turn_on_time.tm_hour);
        printf("TURN ON TIME min READ: %d \n", info.turn_on_time.tm_min);
#endif
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("TURN ON CALENDAR READING FAILED \n");
#endif
    }

    if (read_date_from_flash(aux_str_off, &info.turn_off_time))
    {
#ifdef DEBUG_MODULE
        printf("TURN ON TIME HOUR READ: %d \n", info.turn_off_time.tm_hour);
        printf("TURN ON TIME min READ: %d \n", info.turn_off_time.tm_min);
#endif
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("TURN ON CALENDAR READING FAILED \n");
#endif
    }

    if (read_uint32_from_flash(triac_enable_aux, &value))
    {
        info.enable = (uint8_t)value;
#ifdef DEBUG_MODULE
        printf("TRIAC ENABLE STATUS READ: %d \n", info.enable);
#endif
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("TRIAC ENABLE STATUS READING FAILED \n");
#endif
    }

    global_manager_update_auto_triac_calendar(info, triac_num, true);
}
//------------------------------------------------------------------------------
static void nv_save_triac_calendar(triac_config_info_t triac_info, uint8_t triac_num)
{
    char aux_str_on[20], aux_str_off[20], triac_enable_aux[20];

    switch (triac_num)
    {
    case 1:
        strcpy(aux_str_on, TRIAC1_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC1_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC1_DATE_ENABLE);
        break;
    case 2:
        strcpy(aux_str_on, TRIAC2_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC2_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC2_DATE_ENABLE);
        break;
    case 3:
        strcpy(aux_str_on, TRIAC3_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC3_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC3_DATE_ENABLE);
        break;
    case 4:
        strcpy(aux_str_on, TRIAC4_DATE_ON_KEY);
        strcpy(aux_str_off, TRIAC4_DATE_OFF_KEY);
        strcpy(triac_enable_aux, TRIAC4_DATE_ENABLE);
        break;
    default:
        break;
    }

    write_date_on_flash(aux_str_on, triac_info.turn_on_time);
    write_date_on_flash(aux_str_off, triac_info.turn_off_time);
    write_parameter_on_flash_uint32(triac_enable_aux, (uint32_t)triac_info.enable);
}
//------------------------------------------------------------------------------
static void nv_init_pwm_calendar(void)
{
    pwm_auto_info_t pwm_calendar;
    calendar_auto_mode_t calendar;

    if (read_date_from_flash(PWM_DATE_ON_KEY, &pwm_calendar.turn_on_time))
    {
#ifdef DEBUG_MODULE
        printf("TURN ON TIME HOUR READ: %d \n", pwm_calendar.turn_on_time.tm_hour);
        printf("TURN ON TIME min READ: %d \n", pwm_calendar.turn_on_time.tm_min);
#endif
        calendar.turn_on_time = pwm_calendar.turn_on_time;
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("TURN ON CALENDAR READING FAILED \n");
#endif
    }
    if (read_date_from_flash(PWM_DATE_OFF_KEY, &pwm_calendar.turn_off_time))
    {
#ifdef DEBUG_MODULE
        printf("TURN OFF TIME HOUR READ: %d \n", pwm_calendar.turn_off_time.tm_hour);
        printf("TURN OFF TIME min READ: %d \n", pwm_calendar.turn_off_time.tm_min);
#endif
        calendar.turn_off_time = pwm_calendar.turn_off_time;
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("TURN OFF CALENDAR READING FAILED \n");
#endif
    }

    global_manager_update_auto_pwm_calendar(calendar, true);
}
//------------------------------------------------------------------------------
static void nv_save_pwm_calendar(pwm_auto_info_t pwm_calendar)
{
    write_date_on_flash(PWM_DATE_ON_KEY, pwm_calendar.turn_on_time);
    write_date_on_flash(PWM_DATE_OFF_KEY, pwm_calendar.turn_off_time);
}
//------------------------------------------------------------------------------
static void nv_init_triac_mode(void)
{
    uint32_t value;
    output_mode_t triac_mode;
    if (read_uint32_from_flash(TRIAC_MODE_KEY, &value))
    {
        triac_mode = (output_mode_t)value;
#ifdef DEBUG_MODULE
        printf("TRIAC MODE READ: %d \n", triac_mode);
#endif
        switch (triac_mode)
        {
        case MANUAL_ON:
            global_manager_set_triac_mode_manual_on(true);
            break;
        case MANUAL_OFF:
            global_manager_set_triac_mode_off(true);
            break;
        case AUTOMATIC:
            global_manager_set_triac_mode_auto(true);
            break;
        default:
            break;
        }
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("TRIAC MODE READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_triac_mode(output_mode_t triac_mode)
{
    write_parameter_on_flash_uint32(TRIAC_MODE_KEY, (uint32_t)triac_mode);
}
//------------------------------------------------------------------------------
static void nv_init_pwm_mode(void)
{
    uint32_t value;
    output_mode_t pwm_mode;
    if (read_uint32_from_flash(PWM_MODE_KEY, &value))
    {
        pwm_mode = (output_mode_t)value;
#ifdef DEBUG_MODULE
        printf("PWM MODE READ: %d \n", pwm_mode);
#endif
        global_manager_set_pwm_mode(pwm_mode);
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("PWM MODE READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_pwm_mode(output_mode_t pwm_mode)
{
    write_parameter_on_flash_uint32(PWM_MODE_KEY, (uint32_t)pwm_mode);
}
//------------------------------------------------------------------------------
static void nv_init_simul_day_status(void)
{
    uint32_t value;
    simul_day_status_t simul_day_status;
    if (read_uint32_from_flash(SIMUL_DAY_STATUS_KEY, &value))
    {
        simul_day_status = (simul_day_status_t)value;
#ifdef DEBUG_MODULE
        printf("SIMUL DAY STATUS READ: %d \n", simul_day_status);
#endif
        global_manager_update_simul_day_function_status(simul_day_status, true);
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("SIMUL DAY STATUS READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_simul_day_status(simul_day_status_t simul_day_status)
{
    write_parameter_on_flash_uint32(SIMUL_DAY_STATUS_KEY, (uint32_t)simul_day_status);
}
//------------------------------------------------------------------------------
static void nv_init_rele_vege_status(void)
{
    uint32_t value;

    if (read_uint32_from_flash(RELE_VEGE_STATUS_KEY, &value))
    {
#ifdef DEBUG_MODULE
        printf("RELE VEGE STATUS READ: %lu \n", value);
#endif

        if (value == 0)
        {
            global_manager_set_rele_vege_status_on(true);
        }
        else
        {
            global_manager_set_rele_vege_status_off(true);
        }
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("RELE VEGE STATUS READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_rele_vege_status(rele_output_status_t rele_vege_status)
{
    write_parameter_on_flash_uint32(RELE_VEGE_STATUS_KEY, (uint32_t)rele_vege_status);
}
//------------------------------------------------------------------------------
static void nv_init_auto_percent_power(void)
{
    uint32_t value;
    if (read_uint32_from_flash(PWM_PERCENT_POWER_KEY, &value))
    {
#ifdef DEBUG_MODULE
        printf("AUTO PERCENT POWER READ: %ld \n", value);
#endif
        global_manager_set_pwm_power_value_auto((uint8_t)value, true);
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("AUTO PERCENT POWER READING FAILED \n");
#endif
    }
}
//------------------------------------------------------------------------------
static void nv_save_auto_percent_power(uint8_t percent_power)
{
    write_parameter_on_flash_uint32(PWM_PERCENT_POWER_KEY, (uint32_t)percent_power);
}
//------------------------------------------------------------------------------
// TO DO: Falta agregar la dinamica de funcionamiento del rele vege
static void global_manager_task(void *arg)
{
    global_event_t global_ev;
    nv_info_t global_info;
    pwm_auto_info_t pwm_auto_info;
    uint8_t triac_index;
    bool pwm_auto_status = false;
    bool triac_auto_status = false;
    response_event_t resp_ev;

    global_info.pwm_manual_percent_power = 10;
    global_info.triac_auto.output_status = TRIAC_OUTPUT_OFF;

    // INIT FROM FLASH
    nv_init_ssid_ap_wifi();
    nv_init_password_ap_wifi();
    nv_init_pwm_calendar();
    nv_init_pwm_calendar(); // workaround
    nv_init_pwm_mode();
    nv_init_triac_mode();
    nv_init_simul_day_status();
    nv_init_rele_vege_status();
    nv_init_auto_percent_power();
    nv_init_triac_calendar(1);
    nv_init_triac_calendar(2);
    nv_init_triac_calendar(3);
    nv_init_triac_calendar(4);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ////////////////////////////////////////////////////////
    while (1)
    {
        if (xQueueReceive(global_manager_queue, &global_ev, 50 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch (global_ev.cmd)
            {
            case CMD_UNDEFINED:
                break;
            case GET_CONFIG_NET_INFO:
                strcpy(resp_ev.ssid, global_info.wifi_ssid);
                strcpy(resp_ev.password, global_info.wifi_password);
                xQueueSend(response_queue, &resp_ev, 10);
                break;
            case GET_CONFIG_PWM_INFO:
                resp_ev.pwm_mode = global_info.pwm_mode;
                resp_ev.pwm_auto = global_info.pwm_auto;
                if (global_info.pwm_mode == AUTOMATIC)
                {
                    resp_ev.pwm_auto.percent_power = global_info.pwm_auto.percent_power;
                }
                else if (global_info.pwm_mode == MANUAL_ON)
                {
                    resp_ev.pwm_auto.percent_power = global_info.pwm_manual_percent_power;
                }
                else
                {
                    resp_ev.pwm_auto.percent_power = 0;
                }
                ESP_LOGE("PWM", "la hora del pwm es: %d", global_info.pwm_auto.turn_on_time.tm_hour);
                ESP_LOGE("PWM", "la hora del pwm es: %d", global_info.pwm_auto.turn_on_time.tm_min);
                xQueueSend(response_queue, &resp_ev, 10);
                break;
            case GET_CONFIG_TRIAC_INFO:
                resp_ev.triac_mode = global_info.triac_mode;
                resp_ev.triac_auto = global_info.triac_auto;
                xQueueSend(response_queue, &resp_ev, 10);
                break;
            case GET_CONFIG_RELE_VEGE_INFO:
                resp_ev.rele_vege_status = global_info.rele_vege_status;
                xQueueSend(response_queue, &resp_ev, 10);
                break;
            case UPDATE_CURRENT_TIME:
#ifdef DEBUG_MODULE
                printf("El tiempo actual es: %s", asctime(&global_ev.current_time));
#endif
                global_info.pwm_auto.current_time = global_ev.current_time;
                global_info.triac_auto.current_time = global_ev.current_time;
                break;
            case SET_SSID:
                if ((strcmp((const char *)global_info.wifi_ssid, (const char *)global_ev.str_value) != 0) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_ssid_ap_wifi(global_ev.str_value);
                }
                strcpy(global_info.wifi_ssid, global_ev.str_value);
                break;
            case SET_PASSWORD:
                if ((strcmp((const char *)global_info.wifi_password, (const char *)global_ev.str_value) != 0) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_password_ap_wifi(global_ev.str_value);
                }
                strcpy(global_info.wifi_password, global_ev.str_value);
                break;
            case SET_PWM_MODE:
                global_info.pwm_mode = global_ev.output_mode;
                switch (global_info.pwm_mode)
                {
                case MANUAL_ON:
                    led_manager_pwm_manual_on();
                    pwm_manager_turn_on_pwm(global_info.pwm_manual_percent_power);
                    break;
                case MANUAL_OFF:
                    led_manager_pwm_manual_off();
                    pwm_manager_turn_off_pwm();
                    break;
                case AUTOMATIC:
                    led_manager_pwm_auto();
                    pwm_auto_start();
                    break;
                default:
                    break;
                }
                break;
            case PWM_MANUAL_ON:
                if (global_info.pwm_mode != MANUAL_ON)
                {
                    nv_save_pwm_mode(MANUAL_ON);
                }
                global_info.pwm_mode = MANUAL_ON;
                led_manager_pwm_manual_on();
                pwm_manager_turn_on_pwm(global_info.pwm_manual_percent_power);
                break;
            case PWM_OFF:
                if (global_info.pwm_mode != MANUAL_OFF)
                {
                    nv_save_pwm_mode(MANUAL_OFF);
                }
                global_info.pwm_mode = MANUAL_OFF;
                led_manager_pwm_manual_off();
                pwm_manager_turn_off_pwm();
                break;
            case PWM_AUTO:
                if (global_info.pwm_mode != AUTOMATIC)
                {
                    nv_save_pwm_mode(AUTOMATIC);
                }
                global_info.pwm_mode = AUTOMATIC;
                pwm_auto_start();
                led_manager_pwm_auto();
                break;
            case TRIAC_MANUAL_ON:
                if ((global_info.triac_mode != MANUAL_ON) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_triac_mode(MANUAL_ON);
                }
                global_info.triac_mode = MANUAL_ON;
                led_manager_triac_on();
                triac_manager_turn_on_triac();
                global_info.triac_auto.output_status = TRIAC_OUTPUT_ON;
                break;
            case TRIAC_OFF:
                if ((global_info.triac_mode != MANUAL_OFF) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_triac_mode(MANUAL_OFF);
                }
                global_info.triac_mode = MANUAL_OFF;
                global_info.triac_auto.output_status = TRIAC_OUTPUT_OFF;
                led_manager_triac_off();
                triac_manager_turn_off_triac();

                break;
            case TRIAC_AUTO:
                if ((global_info.triac_mode != AUTOMATIC) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_triac_mode(AUTOMATIC);
                }
                global_info.triac_mode = AUTOMATIC;
                led_manager_triac_auto();
                global_info.triac_auto.output_status = TRIAC_OUTPUT_OFF;
                led_manager_triac_off();
                triac_manager_turn_off_triac();
                break;
            case RELE_VEGE_ON:
                if ((global_info.rele_vege_status != RELE_VEGE_ENABLE) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_rele_vege_status(RELE_VEGE_ENABLE);
                }
                global_info.rele_vege_status = RELE_VEGE_ENABLE;
                led_manager_rele_vege_on();
                vege_manager_turn_on();
                break;
            case RELE_VEGE_OFF:
                if ((global_info.rele_vege_status != RELE_VEGE_DISABLE) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_rele_vege_status(RELE_VEGE_DISABLE);
                }
                global_info.rele_vege_status = RELE_VEGE_DISABLE;
                led_manager_rele_vege_off();
                vege_manager_turn_off();
                break;
            case SET_MANUAL_PWM_POWER:

                if ((global_info.pwm_mode == MANUAL_ON) && (global_ev.value != global_info.pwm_manual_percent_power))
                {
                    pwm_manager_update_pwm(global_ev.value);
#ifdef DEBUG_MODULE
                    printf("UPDATE PWM: %d \n", global_info.pwm_manual_percent_power);
#endif
                }
                global_info.pwm_manual_percent_power = global_ev.value;
                break;
            case SET_AUTO_PWM_POWER:
                if ((global_info.pwm_auto.percent_power != global_ev.value) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_auto_percent_power(global_ev.value);
                }
                global_info.pwm_auto.percent_power = global_ev.value;
                break;
            case UPDATE_SIMUL_DAY_FUNCTION_STATUS:
                if ((global_info.pwm_auto.simul_day_status != global_ev.simul_day_function_status) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_simul_day_status(global_ev.simul_day_function_status);
                }
                global_info.pwm_auto.simul_day_status = global_ev.simul_day_function_status;
                break;
            case UPDATE_PWM_CALENDAR:
                //if (((check_30_min_difference_between_hours(global_ev.pwm_turn_off_time, global_ev.pwm_turn_on_time)) && (global_info.pwm_auto.simul_day_status == SIMUL_DAY_ON)) || (global_info.pwm_auto.simul_day_status == SIMUL_DAY_OFF))
                //{
                    if (((global_ev.pwm_turn_on_time.tm_hour != global_info.pwm_auto.turn_on_time.tm_hour) || (global_ev.pwm_turn_on_time.tm_min != global_info.pwm_auto.turn_on_time.tm_min) || (global_ev.pwm_turn_off_time.tm_hour != global_info.pwm_auto.turn_off_time.tm_hour) || (global_ev.pwm_turn_off_time.tm_min != global_info.pwm_auto.turn_off_time.tm_min)) && (global_ev.value_read_from_flash == false))
                    {
                        pwm_auto_info.turn_on_time = global_ev.pwm_turn_on_time;
                        pwm_auto_info.turn_off_time = global_ev.pwm_turn_off_time;
                        nv_save_pwm_calendar(pwm_auto_info);
                    }
                    global_info.pwm_auto.turn_on_time = global_ev.pwm_turn_on_time;
                    global_info.pwm_auto.turn_off_time = global_ev.pwm_turn_off_time;
                //}

#ifdef DEBUG_MODULE
                // printf("PWM ON calendar: %s", asctime(&global_info.pwm_auto.turn_on_time));
                // printf("PWM OFF calendar: %s", asctime(&global_info.pwm_auto.turn_off_time));
#endif
                break;
            case UPDATE_TRIAC_CALENDAR:
                triac_index = global_ev.triac_num - 1;

                if (((global_ev.triac_info.turn_on_time.tm_hour != global_info.triac_auto.triac_auto[triac_index].turn_on_time.tm_hour) || (global_ev.triac_info.turn_on_time.tm_min != global_info.triac_auto.triac_auto[triac_index].turn_on_time.tm_min) || (global_ev.triac_info.turn_off_time.tm_hour != global_info.triac_auto.triac_auto[triac_index].turn_off_time.tm_hour) || (global_ev.triac_info.turn_off_time.tm_min != global_info.triac_auto.triac_auto[triac_index].turn_off_time.tm_min) || (global_ev.triac_info.enable != global_info.triac_auto.triac_auto[triac_index].enable)) && (global_ev.value_read_from_flash == false))
                {
                    nv_save_triac_calendar(global_ev.triac_info, global_ev.triac_num);
                }
                global_info.triac_auto.triac_auto[triac_index].enable = global_ev.triac_info.enable;
                ESP_LOGE("GLOBALMANAGER", " EL ENABLE DEL 1 ES %d", global_ev.triac_info.enable);
                global_info.triac_auto.triac_auto[triac_index].turn_off_time = global_ev.triac_info.turn_off_time;
                global_info.triac_auto.triac_auto[triac_index].turn_on_time = global_ev.triac_info.turn_on_time;
                break;
            default:
                break;
            }
        }
        else
        {
            if (global_info.pwm_mode == AUTOMATIC)
                pwm_auto_status = true;
            else
                pwm_auto_status = false;

            if (global_info.triac_mode == AUTOMATIC)
                triac_auto_status = true;
            else
                triac_auto_status = false;

            pwm_auto_manager_handler(&global_info.pwm_auto, pwm_auto_status);
            triac_auto_manager_handler(&global_info.triac_auto, triac_auto_status);
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void global_manager_init(void)
{
    global_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(global_event_t));
    response_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(response_event_t));

    xTaskCreate(global_manager_task, "global_manager_task", configMINIMAL_STACK_SIZE * 15,
                NULL, configMAX_PRIORITIES - 1, NULL);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_mode(output_mode_t pwm_mode)
{
    global_event_t ev;
    ev.cmd = SET_PWM_MODE;
    ev.output_mode = pwm_mode;
    xQueueSend(global_manager_queue, &ev, 10);
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
void global_manager_set_triac_mode_off(bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = TRIAC_OFF;
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_triac_mode_manual_on(bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = TRIAC_MANUAL_ON;
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_triac_mode_auto(bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = TRIAC_AUTO;
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_rele_vege_status_off(bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = RELE_VEGE_OFF;
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_rele_vege_status_on(bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = RELE_VEGE_ON;
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_power_value_manual(uint8_t power_percentage_value)
{
    global_event_t ev;
    if (power_percentage_value >= 100)
        power_percentage_value = 100;
    ev.cmd = SET_MANUAL_PWM_POWER;
    ev.value = power_percentage_value;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_set_pwm_power_value_auto(uint8_t power_percentage_value, bool read_from_flash)
{
    global_event_t ev;
    if (power_percentage_value >= 100)
        power_percentage_value = 100;
    ev.cmd = SET_AUTO_PWM_POWER;
    ev.value_read_from_flash = read_from_flash;
    ev.value = power_percentage_value;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_update_current_time(struct tm current_time)
{
    global_event_t ev;
    ev.cmd = UPDATE_CURRENT_TIME;
    ev.current_time = current_time;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void global_manager_update_simul_day_function_status(simul_day_status_t status, bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = UPDATE_SIMUL_DAY_FUNCTION_STATUS;
    ev.value_read_from_flash = read_from_flash;
    ev.simul_day_function_status = status;
    xQueueSend(global_manager_queue, &ev, 10);
}
//------------------------------------------------------------------------------
uint8_t global_manager_set_wifi_ssid(char *wifi_ssid, bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = SET_SSID;
    memset(ev.str_value, '\0', sizeof(ev.str_value));
    strncpy(ev.str_value, wifi_ssid, strlen(wifi_ssid));
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
    return 1;
}
//------------------------------------------------------------------------------
uint8_t global_manager_set_wifi_password(char *wifi_password, bool read_from_flash)
{
    global_event_t ev;
    ev.cmd = SET_PASSWORD;
    memset(ev.str_value, '\0', sizeof(ev.str_value));
    strncpy(ev.str_value, wifi_password, strlen(wifi_password));
    ev.value_read_from_flash = read_from_flash;
    xQueueSend(global_manager_queue, &ev, 10);
    return 1;
}
//------------------------------------------------------------------------------
void global_manager_update_auto_pwm_calendar(calendar_auto_mode_t calendar, bool read_from_flash)
{
    global_event_t ev;
    //if (check_later_hour(calendar.turn_off_time, calendar.turn_on_time))
    {
        ev.cmd = UPDATE_PWM_CALENDAR;
        ev.value_read_from_flash = read_from_flash;
        ev.pwm_turn_on_time = calendar.turn_on_time;
        ev.pwm_turn_off_time = calendar.turn_off_time;
        xQueueSend(global_manager_queue, &ev, 10);
    }
}
//------------------------------------------------------------------------------
void global_manager_update_auto_triac_calendar(triac_config_info_t triac_info, uint8_t triac_num, bool read_from_flash)
{
    global_event_t ev;
    //if (check_later_hour(triac_info.turn_off_time, triac_info.turn_on_time))
    {
        ev.cmd = UPDATE_TRIAC_CALENDAR;
        ev.value_read_from_flash = read_from_flash;
        ev.triac_info.enable = triac_info.enable;
        ESP_LOGE("ASD", " EL ENABLE DEL 1 ES %d asd", triac_info.enable);
        ev.triac_info.turn_off_time = triac_info.turn_off_time;
        ESP_LOGE("ASD", " hora de off %d", triac_info.turn_off_time.tm_hour);
        ev.triac_info.turn_on_time = triac_info.turn_on_time;
        ev.triac_num = triac_num;
        xQueueSend(global_manager_queue, &ev, 10);
    }
}
//------------------------------------------------------------------------------
static void get_net_info(void)
{
    global_event_t ev;
    ev.cmd = GET_CONFIG_NET_INFO;
    xQueueSend(global_manager_queue, &ev, 10);
}

static uint8_t wait_net_info_response(char *ssid, char *password)
{
    response_event_t resp_ev;
    if (xQueueReceive(response_queue, &resp_ev, TIMEOUT_MS / portTICK_PERIOD_MS))
    {
        strcpy(ssid, resp_ev.ssid);
        strcpy(password, resp_ev.password);

        return 1;
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("Error al recibir la respuesta de net\n");
#endif
        return 0;
    }
}

uint8_t global_manager_get_net_info(char *ssid, char *password)
{
    get_net_info();
    if (wait_net_info_response(ssid, password))
    {
        return (1);
    }
    return (0);
}
//------------------------------------------------------------------------------
static void get_pwm_info(void)
{
    global_event_t ev;
    ev.cmd = GET_CONFIG_PWM_INFO;
    xQueueSend(global_manager_queue, &ev, 10);
}

static uint8_t wait_pwm_info_response(output_mode_t *pwm_mode, pwm_auto_info_t *pwm_auto)
{
    response_event_t resp_ev;
    if (xQueueReceive(response_queue, &resp_ev, TIMEOUT_MS / portTICK_PERIOD_MS))
    {
        *pwm_auto = resp_ev.pwm_auto;
        *pwm_mode = resp_ev.pwm_mode;

        return 1;
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("Error al recibir la respuesta de pwm\n");
#endif
        return 0;
    }
}

uint8_t global_manager_get_pwm_info(output_mode_t *pwm_mode, pwm_auto_info_t *pwm_auto)
{
    get_pwm_info();
    if (wait_pwm_info_response(pwm_mode, pwm_auto))
    {
        return (1);
    }
    return (0);
}
//------------------------------------------------------------------------------
static void get_triac_info(void)
{
    global_event_t ev;
    ev.cmd = GET_CONFIG_TRIAC_INFO;
    xQueueSend(global_manager_queue, &ev, 10);
}

static uint8_t wait_triac_info_response(output_mode_t *triac_mode, triac_auto_info_t *triac_auto)
{
    response_event_t resp_ev;
    if (xQueueReceive(response_queue, &resp_ev, TIMEOUT_MS / portTICK_PERIOD_MS))
    {
        *triac_mode = resp_ev.triac_mode;
        *triac_auto = resp_ev.triac_auto;

        return 1;
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("Error al recibir la respuesta de pwm\n");
#endif
        return 0;
    }
}

uint8_t global_manager_get_triac_info(output_mode_t *triac_mode, triac_auto_info_t *triac_auto)
{
    get_triac_info();
    if (wait_triac_info_response(triac_mode, triac_auto))
    {
        return (1);
    }
    return (0);
}
//------------------------------------------------------------------------------
static void get_rele_vege_info(void)
{
    global_event_t ev;
    ev.cmd = GET_CONFIG_RELE_VEGE_INFO;
    xQueueSend(global_manager_queue, &ev, 10);
}

static uint8_t wait_rele_vege_info_response(rele_output_status_t *rele_vege_status)
{
    response_event_t resp_ev;
    if (xQueueReceive(response_queue, &resp_ev, TIMEOUT_MS / portTICK_PERIOD_MS))
    {
        *rele_vege_status = resp_ev.rele_vege_status;
        return 1;
    }
    else
    {
#ifdef DEBUG_MODULE
        printf("Error al recibir la respuesta de pwm\n");
#endif
        return 0;
    }
}

uint8_t global_manager_get_rele_vege_info(rele_output_status_t *rele_vege_status)
{
    get_rele_vege_info();
    if (wait_rele_vege_info_response(rele_vege_status))
    {
        return (1);
    }
    return (0);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------
