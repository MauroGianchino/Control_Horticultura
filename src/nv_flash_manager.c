//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "../include/board_def.h"
#include "../include/nv_flash_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void first_time_flash(void);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
// TO DO: resolver el problema de la primera lectura de date erronea en flash

static void first_time_flash(void)
{
    struct tm time_info_default;

    time_info_default.tm_year = 123;
    time_info_default.tm_mon = 6;
    time_info_default.tm_mday = 8;
    time_info_default.tm_hour = 13;
    time_info_default.tm_min = 30;
    time_info_default.tm_sec = 30;

    init_parameter_in_flash_str(DEVICE_ALIAS_KEY, DEVICE_ALIAS_DEFAULT);
    init_parameter_in_flash_str(WIFI_AP_SSID_KEY, WIFI_AP_SSID_DEFAULT);
    init_date_parameter_in_flash(PWM_DATE_OFF_KEY, time_info_default);// work around
    init_date_parameter_in_flash(PWM_DATE_ON_KEY, time_info_default);
    init_date_parameter_in_flash(TRIAC1_DATE_ON_KEY, time_info_default);
    time_info_default.tm_hour++;
    init_date_parameter_in_flash(PWM_DATE_OFF_KEY, time_info_default);
    init_date_parameter_in_flash(TRIAC1_DATE_OFF_KEY, time_info_default);
    time_info_default.tm_hour++;
    init_date_parameter_in_flash(TRIAC2_DATE_ON_KEY, time_info_default);
    time_info_default.tm_hour++;
    init_date_parameter_in_flash(TRIAC2_DATE_OFF_KEY, time_info_default);
    init_parameter_in_flash_uint32(PWM_MODE_KEY, PWM_MODE_DEFAULT);
    init_parameter_in_flash_uint32(SIMUL_DAY_STATUS_KEY, PWM_SIMUL_DAY_STATUS_DEFAULT);
    init_parameter_in_flash_uint32(PWM_PERCENT_POWER_KEY, PWM_PERCENT_POWER_DEFAULT);
    init_parameter_in_flash_uint32(TRIAC_MODE_KEY, TRIAC_MODE_DEFAULT);
    init_parameter_in_flash_uint32(RELE_VEGE_STATUS_KEY, RELE_VEGE_STATUS_DEFAULT);
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void nv_flash_manager_init(void)
{
    nv_flash_driver_init();

    nv_flash_driver_install_flash();

    first_time_flash();
}
//------------------------------------------------------------------------------
void init_date_parameter_in_flash(char *key, struct tm time_info_default)
{
    char buffer[50];

    memset(buffer, 0, sizeof(buffer));

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time_info_default);
    init_parameter_in_flash_str(key, buffer);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
uint8_t read_date_from_flash(char *key, struct tm *time_info)
{
    char buffer[50];

    memset(buffer, 0, sizeof(buffer));

    read_parameter_from_flash_str(key);
    if(wait_for_flash_response_str(buffer))
    {
        if(strptime(buffer, "%Y-%m-%d %H:%M:%S", time_info) == NULL) 
        {
            printf("Error al convertir la cadena a struct tm.\n");
        }
        return 1;
    }
    return 0;
}
//------------------------------------------------------------------------------
void write_date_on_flash(char *key, struct tm time_info)
{
    char buffer[50];

    memset(buffer, 0, sizeof(buffer));

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time_info);
    write_parameter_on_flash_str(key, buffer);  
}
//------------------------------------------------------------------------------
uint8_t read_uint32_from_flash(char *key, uint32_t *value)
{
    read_parameter_from_flash_uint32(key);
    if(wait_for_flash_response_uint32(value))
    {
        return 1;
    }
    return 0;
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------