#ifndef NV_FLASH_MANAGER_H__
#define NV_FLASH_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <stdint.h>
#include <time.h>

#include "../include/nv_flash_driver.h"
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
// FLASH KEYs
// Tamaño maximo de key 15 caracteres
#define PWM_MODE_KEY "pwm_mode_key\0" 
#define SIMUL_DAY_STATUS_KEY "sim_day_key\0"
#define PWM_PERCENT_POWER_KEY "pwm_power_key\0"
#define TRIAC_MODE_KEY "triac_mod_key\0" 
#define PWM_DATE_ON_KEY "pwm_on_key\0" 
#define PWM_DATE_OFF_KEY "pwm_off_key\0"
#define RELE_VEGE_STATUS_KEY "rele_vege_key\0"
#define TRIAC1_DATE_ON_KEY "triac1_on\0" 
#define TRIAC1_DATE_OFF_KEY "triac1_off\0"
#define TRIAC1_DATE_ENABLE "triac1_enable\0"
#define TRIAC2_DATE_ON_KEY "triac2_on\0" 
#define TRIAC2_DATE_OFF_KEY "triac2_off\0"
#define TRIAC2_DATE_ENABLE "triac2_enable\0"
#define TRIAC3_DATE_ON_KEY "triac3_on\0" 
#define TRIAC3_DATE_OFF_KEY "triac3_off\0"
#define TRIAC3_DATE_ENABLE "triac3_enable\0"
#define TRIAC4_DATE_ON_KEY "triac4_on\0" 
#define TRIAC4_DATE_OFF_KEY "triac4_off\0"
#define TRIAC4_DATE_ENABLE "triac4_enable\0"

#define WIFI_AP_SSID_KEY "ssid_ap_key\0"
#define WIFI_AP_PASSWORD_KEY "pass_ap_key\0"
#define CURRENT_TIME_KEY "actual_time"

// DEFAULT VALUES
#define WIFI_AP_SSID_DEFAULT "Lumenar01" // el tamaño maximo del ssid no debe exceder los 32 caracteres
#define WIFI_AP_PASSWORD_DEFAULT "12345678"
#define PWM_MODE_DEFAULT 2 // MANUAL_OFF
#define PWM_SIMUL_DAY_STATUS_DEFAULT 0 // SIMUL_DAY_OFF
#define PWM_PERCENT_POWER_DEFAULT 50 // 50 percent
#define TRIAC_MODE_DEFAULT 2 // MANUAL_OFF
#define RELE_VEGE_STATUS_DEFAULT 0 // OFF
#define TRIAC1_DATE_ENABLE_DEFAULT 1 // enable
#define TRIAC2_DATE_ENABLE_DEFAULT 0 // disable
#define TRIAC3_DATE_ENABLE_DEFAULT 0 // disable
#define TRIAC4_DATE_ENABLE_DEFAULT 0 // disable
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void nv_flash_manager_init(void);

void init_date_parameter_in_flash(char *key, struct tm time_info_default);
uint8_t read_date_from_flash(char *key, struct tm *time_info);
void write_date_on_flash(char *key, struct tm time_info);
uint8_t read_uint32_from_flash(char *key, uint32_t *value);
uint8_t read_str_from_flash(char *key, char *str_val);
//------------------------------------------------------------------------------
/*
Ejemplo de lectura de una fecha de dataflash

struct tm time_info_aux;
    if(read_date_from_flash(TRIAC2_DATE_OFF_KEY, &time_info_aux))
    {
        printf("Año: %d\n", time_info_aux.tm_year + 1900);
        printf("Mes: %d\n", time_info_aux.tm_mon + 1);
        printf("Día: %d\n", time_info_aux.tm_mday);
        printf("Hora: %d\n", time_info_aux.tm_hour);
        printf("Minuto: %d\n", time_info_aux.tm_min);
        printf("Segundo: %d\n", time_info_aux.tm_sec);
    }
*/
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* NV_FLASH_MANAGER_H__ */